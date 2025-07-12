#include "ui.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "fonts.h"
#include "frame.h"
#include "handles.h"
#include "meshes.h"
#include "render.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/handles.h"
#include "vulkan/textures.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <utility>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

namespace {
  ArenaHandle mem_render = arena::by_name("render");

  ui::UiBuilderFn _ui_builder_fn;

  SDL_Window* _sdl_window = nullptr;

  vulkan::PipelineHandle _ui_pipeline;

  const U32 NUM_CIRCLE_SEGMENTS = 16;

  auto _fonts = A_DARRAY(FontHandle, mem_render);

  vulkan::UBOHandle _global_ubo;
  vulkan::UBOHandle _texture_ubo;

  struct ui_frame {
    vulkan::VertexBufferHandle vertex_buffer;
    vulkan::Vertex2DColorTex*  vertex_mapped_memory;
    U64                        vertex_offset;
    vulkan::IndexBufferHandle  index_buffer;
    U32*                       index_mapped_memory;
    U64                        index_offset;
  } _ui_frames[3];

  struct Rect {
    F32 x;
    F32 y;
    F32 w;
    F32 h;
  };

  MeshHandle _create_mesh(vulkan::Vertex2DColorTex* vertices,
                          U32                       vertex_count,
                          U32*                      indices,
                          U32                       index_count) {
    auto current_ui_frame = &_ui_frames[render::frame::current->index];

    memcpy(current_ui_frame->vertex_mapped_memory + current_ui_frame->vertex_offset,
           vertices,
           vertex_count * sizeof(vulkan::Vertex2DColorTex));

    if (index_count > 0) {
      memcpy(current_ui_frame->index_mapped_memory + current_ui_frame->index_offset,
             indices,
             index_count * sizeof(U32));
    }

    auto mesh_handle =
        meshes::create(current_ui_frame->vertex_buffer,
                       current_ui_frame->index_buffer,
                       vertex_count,
                       index_count,
                       current_ui_frame->vertex_offset * sizeof(vulkan::Vertex2DColorTex),
                       current_ui_frame->index_offset * sizeof(U32));

    current_ui_frame->vertex_offset += vertex_count;
    current_ui_frame->index_offset += index_count;

    return mesh_handle;
  }

  static inline Clay_Dimensions
  clay_measure_text_fn(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    auto font = render::fonts::font(FontHandle{.value = config->fontId});

    F32 x = 0.0f, y = 0.0f;
    F32 max_x = 0.0f;
    F32 max_y = 0.0f;

    for (U32 i = 0; i < text.length; ++i) {
      U8 cp = static_cast<U8>(text.chars[i]);
      if (cp < font.first_codepoint || cp >= font.first_codepoint + font.codepoint_count) {
        continue;
      }

      stbtt_packedchar* ch = &font.packed_chars[cp - font.first_codepoint];

      F32 x0 = x + ch->xoff;
      F32 y0 = y + ch->yoff;
      F32 x1 = x0 + (ch->x1 - ch->x0);
      F32 y1 = y0 + (ch->y1 - ch->y0);

      if (x1 > max_x) max_x = x1;
      if (y1 > max_y) max_y = y1;

      x += ch->xadvance;
    }

    return Clay_Dimensions{max_x, max_y};
  }

  void handle_clay_errors_cb(Clay_ErrorData errorData) {
    printf("clay error: %s", errorData.errorText.chars);
  }

  void draw_fill_rounded_rect(const Rect rect, F32 corner_radius, glm::vec4 color) {
    U32 index_count = 0, vertex_count = 0;

    F32 min_radius     = std::min(rect.w, rect.h) / 2.0f;
    F32 clamped_radius = std::min(corner_radius, min_radius);

    U32 num_circle_segments =
        std::max(NUM_CIRCLE_SEGMENTS, static_cast<U32>(clamped_radius * 0.5f));

    U32 total_vertices = 4 + (4 * (num_circle_segments * 2)) + 2 * 4;
    U32 total_indices  = 6 + (4 * (num_circle_segments * 3)) + 6 * 4;

    vulkan::Vertex2DColorTex vertices[total_vertices];
    U32                      indices[total_indices];

    // define center rectangle
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + clamped_radius, rect.y + clamped_radius},
        color,
        {0, 0},
    };

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w - clamped_radius, rect.y + clamped_radius},
        color,
        {1, 0},

    };

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w - clamped_radius, rect.y + rect.h - clamped_radius},
        color,
        {1, 1},
    };

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + clamped_radius, rect.y + rect.h - clamped_radius},
        color,
        {0, 1},
    };

    indices[index_count++] = 0;
    indices[index_count++] = 3;
    indices[index_count++] = 1;
    indices[index_count++] = 1;
    indices[index_count++] = 3;
    indices[index_count++] = 2;

    // define rounded corners as triangle fans
    const F32 step = (glm::pi<F32>() / 2) / num_circle_segments;

    for (int i = 0; i < num_circle_segments; i++) {
      const F32 angle1 = (F32)i * step;
      const F32 angle2 = ((F32)i + 1.0f) * step;

      // Corner 0: top-left
      F32 cx = rect.x + clamped_radius;
      F32 cy = rect.y + clamped_radius;
      // emit two arc vertices
      vertices[vertex_count++] = {{cx + glm::cos(angle1) * clamped_radius * (-1),
                                   cy + glm::sin(angle1) * clamped_radius * (-1)},
                                  color,
                                  {0, 0}};
      vertices[vertex_count++] = {{cx + glm::cos(angle2) * clamped_radius * (-1),
                                   cy + glm::sin(angle2) * clamped_radius * (-1)},
                                  color,
                                  {0, 0}};
      indices[index_count++]   = 0;
      indices[index_count++]   = vertex_count - 1;
      indices[index_count++]   = vertex_count - 2;

      //  Corner 1: top-right
      cx                       = rect.x + rect.w - clamped_radius;
      cy                       = rect.y + clamped_radius;
      vertices[vertex_count++] = {{cx + glm::cos(angle1) * clamped_radius * (+1),
                                   cy + glm::sin(angle1) * clamped_radius * (-1)},
                                  color,
                                  {0, 0}};
      vertices[vertex_count++] = {{cx + glm::cos(angle2) * clamped_radius * (+1),
                                   cy + glm::sin(angle2) * clamped_radius * (-1)},
                                  color,
                                  {0, 0}};
      // center vertex index = 1
      indices[index_count++] = 1;
      indices[index_count++] = vertex_count - 2;
      indices[index_count++] = vertex_count - 1;

      // Corner 2: bottom-right
      cx                       = rect.x + rect.w - clamped_radius;
      cy                       = rect.y + rect.h - clamped_radius;
      vertices[vertex_count++] = {{cx + glm::cos(angle1) * clamped_radius * (+1),
                                   cy + glm::sin(angle1) * clamped_radius * (+1)},
                                  color,
                                  {0, 0}};
      vertices[vertex_count++] = {{cx + glm::cos(angle2) * clamped_radius * (+1),
                                   cy + glm::sin(angle2) * clamped_radius * (+1)},
                                  color,
                                  {0, 0}};
      // center vertex index = 2
      indices[index_count++] = 2;
      indices[index_count++] = vertex_count - 1;
      indices[index_count++] = vertex_count - 2;

      // Corner 3: bottom-left
      cx                       = rect.x + clamped_radius;
      cy                       = rect.y + rect.h - clamped_radius;
      vertices[vertex_count++] = {{cx + glm::cos(angle1) * clamped_radius * (-1),
                                   cy + glm::sin(angle1) * clamped_radius * (+1)},
                                  color,
                                  {0, 0}};
      vertices[vertex_count++] = {{cx + glm::cos(angle2) * clamped_radius * (-1),
                                   cy + glm::sin(angle2) * clamped_radius * (+1)},
                                  color,
                                  {0, 0}};
      // center vertex index = 3
      indices[index_count++] = 3;
      indices[index_count++] = vertex_count - 2;
      indices[index_count++] = vertex_count - 1;
    }

    // Define edge rectangles
    //  Top edge
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + clamped_radius, rect.y},
        color,
        {0, 0},
    };

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w - clamped_radius, rect.y},
        color,
        {1, 0},
    }; // TR

    indices[index_count++] = 0;
    indices[index_count++] = vertex_count - 1; // TL
    indices[index_count++] = vertex_count - 2; // TR
    indices[index_count++] = 1;
    indices[index_count++] = vertex_count - 1;
    indices[index_count++] = 0; // TR
                                //
    // Right edge
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w, rect.y + clamped_radius},
        color,
        {1, 0},
    }; // RT

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w, rect.y + rect.h - clamped_radius},
        color,
        {1, 1},
    }; // RB

    indices[index_count++] = 1;
    indices[index_count++] = vertex_count - 1; // RT
    indices[index_count++] = vertex_count - 2; // RB
    indices[index_count++] = 2;
    indices[index_count++] = vertex_count - 1;
    indices[index_count++] = 1; // RB

    // Bottom edge
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + rect.w - clamped_radius, rect.y + rect.h},
        color,
        {1, 1},
    }; // BR

    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x + clamped_radius, rect.y + rect.h},
        color,
        {0, 1},
    }; // BL

    indices[index_count++] = 2;
    indices[index_count++] = vertex_count - 1; // BR
    indices[index_count++] = vertex_count - 2; // BL
    indices[index_count++] = 3;
    indices[index_count++] = vertex_count - 1;
    indices[index_count++] = 2; // BL
                                //
    // Left edge
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x, rect.y + rect.h - clamped_radius},
        color,
        {0, 1},
    }; // LB
    vertices[vertex_count++] = vulkan::Vertex2DColorTex{
        {rect.x, rect.y + clamped_radius},
        color,
        {0, 0},
    }; // LT

    indices[index_count++] = 3;
    indices[index_count++] = vertex_count - 1; // LB
    indices[index_count++] = vertex_count - 2; // LT
    indices[index_count++] = 0;
    indices[index_count++] = vertex_count - 1;
    indices[index_count++] = 3; // LT

    auto mesh = _create_mesh(vertices, total_vertices, indices, total_indices);
    render::draw(mesh);
    meshes::cleanup(mesh, false);
  }

  void draw_fill_rect(const Rect rect, glm::vec4 color) {
    vulkan::Vertex2DColorTex vertices[] = {
        {{rect.x, rect.y}, color, {0, 0}},
        {{rect.x + rect.w, rect.y}, color, {1, 0}},
        {{rect.x + rect.w, rect.y + rect.h}, color, {1, 1}},
        {{rect.x, rect.y + rect.h}, color, {0, 1}},
    };

    U32 indices[] = {0, 3, 1, 1, 3, 2};

    auto mesh = _create_mesh(vertices, 4, indices, 6);
    render::draw(mesh);
    meshes::cleanup(mesh, false);
  }

  void draw_arc(const glm::vec2 center,
                const F32       radius,
                const F32       startAngle,
                const F32       endAngle,
                const F32       thickness,
                const glm::vec4 color) {

    const float rad_start = startAngle * (glm::pi<F32>() / 180.0f);
    const float rad_end   = endAngle * (glm::pi<F32>() / 180.0f);

    const U32 num_circle_segments = std::max(NUM_CIRCLE_SEGMENTS, static_cast<U32>(radius * 1.5f));
    const F32 angle_step          = (rad_end - rad_start) / static_cast<float>(num_circle_segments);

    auto vertices = F_DARRAY_CAP(vulkan::Vertex2DColorTex, (num_circle_segments + 1) * 2);
    auto indices  = F_DARRAY_CAP(U32, num_circle_segments * 6);

    const F32 outer_radius = radius;
    const F32 inner_radius = radius - thickness;

    for (U32 i = 0; i <= num_circle_segments; i++) {
      F32 angle = rad_start + i * angle_step;
      F32 cos_a = std::cos(angle);
      F32 sin_a = std::sin(angle);

      glm::vec2 outer_pos = center + glm::vec2(cos_a, sin_a) * outer_radius;
      glm::vec2 inner_pos = center + glm::vec2(cos_a, sin_a) * inner_radius;

      array::push_back(vertices, vulkan::Vertex2DColorTex{outer_pos, color, {0, 0}});
      array::push_back(vertices, vulkan::Vertex2DColorTex{inner_pos, color, {0, 0}});
    }

    for (U32 i = 0; i < num_circle_segments; i++) {
      U32 outer_curr = i * 2;
      U32 inner_curr = i * 2 + 1;
      U32 outer_next = (i + 1) * 2;
      U32 inner_next = (i + 1) * 2 + 1;

      array::push_back(indices, outer_curr);
      array::push_back(indices, inner_curr);
      array::push_back(indices, outer_next);

      array::push_back(indices, outer_next);
      array::push_back(indices, inner_curr);
      array::push_back(indices, inner_next);
    }

    auto mesh = _create_mesh(vertices._data, vertices._size, indices._data, indices._size);
    render::draw(mesh);
    meshes::cleanup(mesh, false);
  }

  void draw_border(Rect& rect, Clay_BorderRenderData* clay_data) {
    const F32 min_radius = std::min(rect.w, rect.h) / 2.0f;

    const Clay_CornerRadius clamped_radii = {
        .topLeft     = std::min(clay_data->cornerRadius.topLeft, min_radius),
        .topRight    = std::min(clay_data->cornerRadius.topRight, min_radius),
        .bottomLeft  = std::min(clay_data->cornerRadius.bottomLeft, min_radius),
        .bottomRight = std::min(clay_data->cornerRadius.bottomRight, min_radius)};

    glm::vec4 color = {
        clay_data->color.r / 255,
        clay_data->color.g / 255,
        clay_data->color.b / 255,
        clay_data->color.a / 255,
    };

    // edges
    if (clay_data->width.left > 0) {
      const F32 starting_y = rect.y + clamped_radii.topLeft;
      const F32 length     = rect.h - clamped_radii.topLeft - clamped_radii.bottomLeft;

      Rect line = {
          rect.x,
          starting_y,
          static_cast<F32>(clay_data->width.left),
          length,
      };

      draw_fill_rect(line, color);
    }

    if (clay_data->width.right > 0) {
      const F32 starting_x = rect.x + rect.w - static_cast<F32>(clay_data->width.right);
      const F32 starting_y = rect.y + clamped_radii.topRight;
      const F32 length     = rect.h - clamped_radii.topRight - clamped_radii.bottomRight;

      Rect line = {
          starting_x,
          starting_y,
          static_cast<float>(clay_data->width.right),
          length,
      };

      draw_fill_rect(line, color);
    }

    if (clay_data->width.top > 0) {
      const F32 starting_x = rect.x + clamped_radii.topLeft;
      const F32 length     = rect.w - clamped_radii.topLeft - clamped_radii.topRight;

      Rect line = {
          starting_x,
          rect.y,
          length,
          static_cast<float>(clay_data->width.top),
      };

      draw_fill_rect(line, color);
    }

    if (clay_data->width.bottom > 0) {
      const F32 starting_x = rect.x + clamped_radii.bottomLeft;
      const F32 starting_y = rect.y + rect.h - static_cast<F32>(clay_data->width.bottom);
      const F32 length     = rect.w - clamped_radii.bottomLeft - clamped_radii.bottomRight;

      Rect line = {
          starting_x,
          starting_y,
          length,
          static_cast<float>(clay_data->width.bottom),
      };

      draw_fill_rect(line, color);
    }

    // corners
    if (clay_data->cornerRadius.topLeft > 0) {
      const F32 center_x = rect.x + clamped_radii.topLeft;
      const F32 center_y = rect.y + clamped_radii.topLeft;
      draw_arc(glm::vec2{center_x, center_y},
               clamped_radii.topLeft,
               180.0f,
               270.0f,
               clay_data->width.top,
               color);
    }

    if (clay_data->cornerRadius.topRight > 0) {
      const F32 center_x = rect.x + rect.w - clamped_radii.topRight;
      const F32 center_y = rect.y + clamped_radii.topRight;

      draw_arc(glm::vec2{center_x, center_y},
               clamped_radii.topRight,
               270.0f,
               360.0f,
               clay_data->width.top,
               color);
    }

    if (clay_data->cornerRadius.bottomLeft > 0) {
      const F32 center_x = rect.x + clamped_radii.bottomLeft;
      const F32 center_y = rect.y + rect.h - clamped_radii.bottomLeft;
      draw_arc(glm::vec2{center_x, center_y},
               clamped_radii.bottomLeft,
               90.0f,
               180.0f,
               clay_data->width.bottom,
               color);
    }

    if (clay_data->cornerRadius.bottomRight > 0) {
      const F32 center_x = rect.x + rect.w - clamped_radii.bottomRight;
      const F32 center_y = rect.y + rect.h - clamped_radii.bottomRight;
      draw_arc(glm::vec2{center_x, center_y},
               clamped_radii.bottomRight,
               0.0f,
               90.0f,
               clay_data->width.bottom,
               color);
    }
  }
}

void draw_text(Rect& rect, Clay_TextRenderData& clay_data) {
  auto font = render::fonts::font(FontHandle{.value = clay_data.fontId});

  glm::vec4 color = {clay_data.textColor.r / 255.0f,
                     clay_data.textColor.g / 255.0f,
                     clay_data.textColor.b / 255.0f,
                     clay_data.textColor.a / 255.0f};

  auto text =
      string::init(arena::frame(), clay_data.stringContents.chars, clay_data.stringContents.length);

  int quad_count = 0;
  for (int i = 0; i < text._size; ++i) {
    U8 codepoint = text._data[i];

    if (codepoint >= font.first_codepoint &&
        codepoint < font.first_codepoint + font.codepoint_count) {
      ++quad_count;
    }
  }

  if (quad_count == 0) return;

  vulkan::Vertex2DColorTex vertices[quad_count * 4];
  U32                      indices[quad_count * 6];

  U32 vertex_count  = 0;
  U32 indices_count = 0;

  // calc max_glyph_height
  F32 max_glyph_height = 0.0f;
  for (int i = 0; i < text._size; ++i) {
    U8 code_point = (unsigned char)text._data[i];
    if (code_point < font.first_codepoint ||
        code_point >= font.first_codepoint + font.codepoint_count) {
      continue;
    }

    auto* pc = &font.packed_chars[code_point - font.first_codepoint];
    F32   h  = (pc->y1 - pc->y0);

    if (h > max_glyph_height) max_glyph_height = h;
  }

  F32 current_x = 0.0f;
  F32 current_y = max_glyph_height;

  for (int ci = 0; ci < text._size; ++ci) {
    U8 codepoint = (unsigned char)text._data[ci];
    if (codepoint < font.first_codepoint ||
        codepoint >= font.first_codepoint + font.codepoint_count) {
      continue;
    }

    stbtt_packedchar* packed_char = &font.packed_chars[codepoint - font.first_codepoint];

    F32 x0 = current_x + packed_char->xoff;
    F32 y0 = current_y + packed_char->yoff;
    F32 x1 = x0 + (packed_char->x1 - packed_char->x0);
    F32 y1 = y0 + (packed_char->y1 - packed_char->y0);

    F32 s0 = packed_char->x0 / static_cast<F32>(font.bitmap_width);
    F32 t0 = packed_char->y0 / static_cast<F32>(font.bitmap_height);
    F32 s1 = packed_char->x1 / static_cast<F32>(font.bitmap_width);
    F32 t1 = packed_char->y1 / static_cast<F32>(font.bitmap_height);

    // emit quad verts with tex_index
    vertices[vertex_count++] = {{x0, y0}, color, {s0, t0}};
    vertices[vertex_count++] = {{x1, y0}, color, {s1, t0}};
    vertices[vertex_count++] = {{x1, y1}, color, {s1, t1}};
    vertices[vertex_count++] = {{x0, y1}, color, {s0, t1}};

    // emit indices
    U32 base                 = vertex_count - 4;
    indices[indices_count++] = base + 0;
    indices[indices_count++] = base + 2;
    indices[indices_count++] = base + 1;
    indices[indices_count++] = base + 0;
    indices[indices_count++] = base + 3;
    indices[indices_count++] = base + 2;

    current_x += packed_char->xadvance;
  }

  auto mesh = _create_mesh(vertices, vertex_count, indices, indices_count);

  float     scale = clay_data.fontSize / font.pixel_height;
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(rect.x, rect.y, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));
  meshes::set_constants(mesh, model, 0);

  render::draw(mesh);
  meshes::cleanup(mesh, false);
}

void ui::init(SDL_Window*              sdl_window,
              vulkan::RenderPassHandle render_pass,
              DynamicArray<String>&    font_paths) {
  _sdl_window = sdl_window;

  _global_ubo = vulkan::ubos::create_ubo_buffer(0,
                                                vulkan::StageFlags::SHADER_VERTEX,
                                                sizeof(vulkan::GlobalUBO));
  _texture_ubo =
      vulkan::ubos::create_texture_set(1, vulkan::StageFlags::SHADER_FRAGMENT, font_paths._size);

  _ui_pipeline = render::create_pipeline({
      .vertex_shader_fpath                 = "vert_ui.spv",
      .fragment_shader_fpath               = "frag_ui.spv",
      .binding_description_count           = 1,
      .binding_description                 = &vulkan::VERTEX2D_COLOR_TEX_BINDING_DESC,
      .attribute_descriptions              = vulkan::VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC,
      .attribute_descriptions_format_count = array::size(vulkan::VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC),
      .ubos                                = S_DARRAY(vulkan::UBOHandle, _global_ubo, _texture_ubo),
      .disable_depth_testing               = true,
  });

  U64        mem_size    = Clay_MinMemorySize();
  Clay_Arena clay_memory = {
      .capacity = mem_size,
      .memory   = arena::alloc<char>(mem_render, mem_size),
  };

  int width, height;
  SDL_GetWindowSize(sdl_window, &width, &height);
  Clay_Initialize(clay_memory,
                  Clay_Dimensions{static_cast<F32>(width), static_cast<F32>(height)},
                  Clay_ErrorHandler{handle_clay_errors_cb});

  auto font_textures = S_DARRAY_SIZE(TextureHandle, font_paths._size);

  for (U32 i = 0; i < font_paths._size; ++i) {
    auto font              = render::fonts::load(font_paths._data[i]._data);
    font_textures._data[i] = render::fonts::font(font).texture;
    array::push_back(_fonts, font);
  }

  vulkan::ubos::set_textures(_texture_ubo, font_textures);

  Clay_SetMeasureTextFunction(clay_measure_text_fn, nullptr);

  for (U8 i = 0; i < 3; ++i) {
    _ui_frames[i].vertex_buffer = vulkan::vertex_buffers::create(2 * 1024 * 1024);
    _ui_frames[i].vertex_mapped_memory =
        static_cast<vulkan::Vertex2DColorTex*>(vulkan::buffers::map(_ui_frames[i].vertex_buffer));

    _ui_frames[i].index_buffer = vulkan::index_buffers::create(2 * 1024 * 1024);
    _ui_frames[i].index_mapped_memory =
        static_cast<U32*>(vulkan::buffers::map(_ui_frames[i].index_buffer));
  }
}

void ui::cleanup() {
  for (U32 i = 0; i < _fonts._size; ++i) {
    render::fonts::cleanup(_fonts._data[i]);
  }

  for (U8 i = 0; i < 3; ++i) {
    vulkan::vertex_buffers::cleanup(_ui_frames[i].vertex_buffer);
    vulkan::index_buffers::cleanup(_ui_frames[i].index_buffer);
  }

  vulkan::ubos::cleanup(_global_ubo);
  vulkan::ubos::cleanup(_texture_ubo);
}

void ui::draw_frame() {
  if (!_ui_builder_fn) return;

  _ui_frames[render::frame::current->index].vertex_offset = 0;
  _ui_frames[render::frame::current->index].index_offset  = 0;

  auto clay_commands = _ui_builder_fn();

  render::bind_pipeline(_ui_pipeline);

  int width, height;
  SDL_GetWindowSize(_sdl_window, &width, &height);

  render::set_view_projection(
      _global_ubo,
      glm::mat4(1.0f),
      glm::ortho(0.0f, static_cast<F32>(width), static_cast<F32>(height), 0.0f, -1.0f, 1.0f));

  for (size_t i = 0; i < clay_commands.length; i++) {
    auto clay_command = Clay_RenderCommandArray_Get(&clay_commands, i);

    Rect rect = {
        .x = clay_command->boundingBox.x,
        .y = clay_command->boundingBox.y,
        .w = clay_command->boundingBox.width,
        .h = clay_command->boundingBox.height,
    };

    switch (clay_command->commandType) {
      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
        Clay_RectangleRenderData* config = &clay_command->renderData.rectangle;

        glm::vec4 color = {config->backgroundColor.r / 255.0f,
                           config->backgroundColor.g / 255.0f,
                           config->backgroundColor.b / 255.0f,
                           config->backgroundColor.a / 255.0f};

        if (config->cornerRadius.topLeft > 0) {
          draw_fill_rounded_rect(rect, config->cornerRadius.topLeft, color);
        } else {
          draw_fill_rect(rect, color);
        }
      } break;
      case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        draw_text(rect, clay_command->renderData.text);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_BORDER: {
        draw_border(rect, &clay_command->renderData.border);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
        VkRect2D scissor = {
            {
                static_cast<I32>(rect.x),
                static_cast<I32>(rect.y),
            },
            {
                static_cast<U32>(rect.w),
                static_cast<U32>(rect.h),
            },
        };

        auto vk_command_buffer =
            *vulkan::command_buffers::buffer(render::frame::current->command_buffer);

        vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        VkRect2D scissor = {{0, 0}, {UINT32_MAX, UINT32_MAX}};

        auto vk_command_buffer =
            *vulkan::command_buffers::buffer(render::frame::current->command_buffer);

        vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
        // Clay_ImageRenderData* config = &clay_command->renderData.image;

        // if (config->textureId == 0) {
        //   // no texture, skip
        //   break;
        // }
        //
        // auto texture = render::textures::texture(TextureHandle{.value = config->textureId});
        //
        // if (!texture) {
        //   printf("ui: image with id %u not found\n", config->textureId);
        //   break;
        // }
        //
        // vulkan::Vertex2DColorTex vertices[4] = {
        //     {{rect.x, rect.y}, {1, 1, 1, 1}, {0, 0}},
        //     {{rect.x + rect.w, rect.y}, {1, 1, 1, 1}, {1, 0}},
        //     {{rect.x + rect.w, rect.y + rect.h}, {1, 1, 1, 1}, {1, 1}},
        //     {{rect.x, rect.y + rect.h}, {1, 1, 1, 1}, {0, 1}},
        // };
        //
        // U32 indices[] = {0, 3, 1, 1, 3, 2};
        //
        // auto mesh = _create_mesh(vertices, 4, indices, 6);
        //
        // meshes::set_texture(mesh,
        //                     vulkan::textures::handle(texture),
        //                     vulkan::textures::sampler(texture));
        //
        // render::draw(mesh);
        // meshes::cleanup(mesh);
        // TODO: Vulkan texture rendering (bind image and draw quad)
      } break;
      case CLAY_RENDER_COMMAND_TYPE_NONE:
        break;
      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        break;
    }
  }
}

void ui::set_builder(UiBuilderFn ui_builder_fn) { _ui_builder_fn = ui_builder_fn; }
