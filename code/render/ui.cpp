#include "ui.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "handles.h"
#include "meshes.h"
#include "render.h"
#include "vulkan/command_buffers.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

#include <algorithm>
#include <cstdio>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

namespace {
  ArenaHandle mem_ui = arena::by_name("render_ui");

  auto _frame_meshes = array::init<MeshHandle>(mem_ui, 100);

  ui::UiBuilderFn _ui_builder_fn;

  SDL_Window* _sdl_window = nullptr;

  vulkan::PipelineHandle _ui_pipeline;

  const U32 NUM_CIRCLE_SEGMENTS = 16;
  const U32 FONT_ID             = 0;

  struct Rect {
    float x;
    float y;
    float w;
    float h;
  };

  /* static inline Clay_Dimensions */
  /* sdl_measure_text_cb(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) { */
  /*   TTF_Font** fonts = static_cast<TTF_Font**>(userData); */
  /*   TTF_Font*  font  = fonts[config->fontId]; */
  /*   I32        width, height; */
  /*  */
  /*   if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) { */
  /*     SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError()); */
  /*   } */
  /*  */
  /*   return Clay_Dimensions{(F32)width, (F32)height}; */
  /* } */
  /*  */

  void handle_clay_errors_cb(Clay_ErrorData errorData) {
    printf("clay error: %s", errorData.errorText.chars);
  }

  void draw_fill_rounded_rect(vulkan::CommandBufferHandle command_buffer,
                              const Rect                  rect,
                              F32                         corner_radius,
                              glm::vec4                   color) {
    printf("COLOR: %s\n", glm::to_string(color).c_str());

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
    indices[index_count++] = 1;
    indices[index_count++] = 3;
    indices[index_count++] = 1;
    indices[index_count++] = 2;
    indices[index_count++] = 3;

    // define rounded corners as triangle fans
    const F32 step = (glm::pi<F32>() / 2) / num_circle_segments;

    for (int i = 0; i < num_circle_segments; i++) {
      const F32 angle1 = (F32)i * step;
      const F32 angle2 = ((F32)i + 1.0f) * step;

      for (int j = 0; j < 4; j++) { // Iterate over four corners
        F32 cx, cy, sign_x, sign_y;

        switch (j) {
          case 0:
            cx     = rect.x + clamped_radius;
            cy     = rect.y + clamped_radius;
            sign_x = -1;
            sign_y = -1;
            break; // Top-left
          case 1:
            cx     = rect.x + rect.w - clamped_radius;
            cy     = rect.y + clamped_radius;
            sign_x = 1;
            sign_y = -1;
            break; // Top-right
          case 2:
            cx     = rect.x + rect.w - clamped_radius;
            cy     = rect.y + rect.h - clamped_radius;
            sign_x = 1;
            sign_y = 1;
            break; // Bottom-right
          case 3:
            cx     = rect.x + clamped_radius;
            cy     = rect.y + rect.h - clamped_radius;
            sign_x = -1;
            sign_y = 1;
            break; // Bottom-left
          default:
            return;
        }

        vertices[vertex_count++] = vulkan::Vertex2DColorTex{
            {cx + glm::cos<F32>(angle1) * clamped_radius * sign_x,
             cy + glm::sin<F32>(angle1) * clamped_radius * sign_y},
            color,
            {0, 0},
        };

        vertices[vertex_count++] = vulkan::Vertex2DColorTex{
            {cx + glm::cos<F32>(angle2) * clamped_radius * sign_x,
             cy + glm::sin<F32>(angle2) * clamped_radius * sign_y},
            color,
            {0, 0},
        };

        indices[index_count++] = j; // Connect to corresponding central rectangle vertex
        indices[index_count++] = vertex_count - 2;
        indices[index_count++] = vertex_count - 1;
      }
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
    indices[index_count++] = vertex_count - 2; // TL
    indices[index_count++] = vertex_count - 1; // TR
    indices[index_count++] = 1;
    indices[index_count++] = 0;
    indices[index_count++] = vertex_count - 1; // TR
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
    indices[index_count++] = vertex_count - 2; // RT
    indices[index_count++] = vertex_count - 1; // RB
    indices[index_count++] = 2;
    indices[index_count++] = 1;
    indices[index_count++] = vertex_count - 1; // RB

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
    indices[index_count++] = vertex_count - 2; // BR
    indices[index_count++] = vertex_count - 1; // BL
    indices[index_count++] = 3;
    indices[index_count++] = 2;
    indices[index_count++] = vertex_count - 1; // BL
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
    indices[index_count++] = vertex_count - 2; // LB
    indices[index_count++] = vertex_count - 1; // LT
    indices[index_count++] = 0;
    indices[index_count++] = 3;
    indices[index_count++] = vertex_count - 1; // LT

    // Create Vulkan buffers and copy vertex/index data
    auto mesh = meshes::create(vertices, total_vertices, indices, total_indices);
    meshes::draw(command_buffer, mesh);
    array::push_back(_frame_meshes, mesh);
  }

  void
  draw_fill_rect(vulkan::CommandBufferHandle command_buffer, const Rect rect, glm::vec4 color) {
    vulkan::Vertex2DColorTex vertices[] = {
        {{rect.x, rect.y}, color, {0, 0}},
        {{rect.x + rect.w, rect.y}, color, {1, 0}},
        {{rect.x + rect.w, rect.y + rect.h}, color, {1, 1}},
        {{rect.x, rect.y + rect.h}, color, {0, 1}},
    };

    U32 indices[] = {0, 1, 3, 1, 2, 3};

    auto mesh = meshes::create(vertices, 4, indices, 6);
    meshes::draw(command_buffer, mesh);
    array::push_back(_frame_meshes, mesh);
  }

  void draw_arc(vulkan::CommandBufferHandle command_buffer,
                const glm::vec2             center,
                const F32                   radius,
                const F32                   startAngle,
                const F32                   endAngle,
                const F32                   thickness,
                const glm::vec4             color) {
    const float rad_start = startAngle * (glm::pi<F32>() / 180.0f);
    const float rad_end   = endAngle * (glm::pi<F32>() / 180.0f);

    // Determine how many segments to use.
    const U32 num_circle_segments = std::max(NUM_CIRCLE_SEGMENTS, static_cast<U32>(radius * 1.5f));
    const F32 angle_step          = (rad_end - rad_start) / static_cast<float>(num_circle_segments);
    const F32 thickness_step      = 0.4f; // Arbitrary value to space the rings.

    // For each "ring" of the arc (to simulate thickness)...
    for (F32 t = thickness_step; t < thickness - thickness_step; t += thickness_step) {
      // Temporary container for this ring’s vertices.
      printf("CHECK THESE SIZES 100 needed?");
      auto vertices = array::init<vulkan::Vertex2DColorTex>(arena::frame(), 100);

      const F32 clamped_radius = std::max(radius - t, 1.0f);

      // Generate the points along the arc.
      for (U32 i = 0; i <= num_circle_segments; i++) {
        F32 angle = rad_start + i * angle_step;
        F32 x     = center.x + std::cos(angle) * clamped_radius;
        F32 y     = center.y + std::sin(angle) * clamped_radius;
        // Create vertex with position and per-vertex color.
        array::push_back(vertices,
                         vulkan::Vertex2DColorTex{
                             glm::vec2(std::round(x), std::round(y)),
                             color,
                             {0, 0},
                         });
      }

      auto mesh = meshes::create(vertices);
      meshes::draw(command_buffer, mesh);
      array::push_back(_frame_meshes, mesh);
    }
  }

  void draw_border(vulkan::CommandBufferHandle command_buffer,
                   Rect                        rect,
                   Clay_BorderRenderData*      clay_data) {
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

      draw_fill_rect(command_buffer, line, color);
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

      draw_fill_rect(command_buffer, line, color);
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

      draw_fill_rect(command_buffer, line, color);
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

      draw_fill_rect(command_buffer, line, color);
    }

    // corners
    if (clay_data->cornerRadius.topLeft > 0) {
      const F32 center_x = rect.x + clamped_radii.topLeft - 1;
      const F32 center_y = rect.y + clamped_radii.topLeft;
      draw_arc(command_buffer,
               glm::vec2{center_x, center_y},
               clamped_radii.topLeft,
               180.0f,
               270.0f,
               clay_data->width.top,
               color);
    }

    if (clay_data->cornerRadius.topRight > 0) {
      const F32 center_x = rect.x + rect.w - clamped_radii.topRight - 1;
      const F32 center_y = rect.y + clamped_radii.topRight;

      draw_arc(command_buffer,
               glm::vec2{center_x, center_y},
               clamped_radii.topRight,
               270.0f,
               360.0f,
               clay_data->width.top,
               color);
    }

    if (clay_data->cornerRadius.bottomLeft > 0) {
      const F32 center_x = rect.x + clamped_radii.bottomLeft - 1;
      const F32 center_y = rect.y + rect.h - clamped_radii.bottomLeft - 1;
      draw_arc(command_buffer,
               glm::vec2{center_x, center_y},
               clamped_radii.bottomLeft,
               90.0f,
               180.0f,
               clay_data->width.bottom,
               color);
    }

    if (clay_data->cornerRadius.bottomRight > 0) {
      const F32 center_x = rect.x + rect.w - clamped_radii.bottomRight - 1;
      const F32 center_y = rect.y + rect.h - clamped_radii.bottomRight - 1;
      draw_arc(command_buffer,
               glm::vec2{center_x, center_y},
               clamped_radii.bottomRight,
               0.0f,
               90.0f,
               clay_data->width.bottom,
               color);
    }
  }

}

void ui::init(SDL_Window* sdl_window, vulkan::RenderPassHandle render_pass) {
  _sdl_window = sdl_window;

  _ui_pipeline = vulkan::pipelines::by_name("ui_pipeline");

  VkDescriptorSetLayout ubo_layouts[] = {
      vulkan::ubos::global_ubo_layout(),
      vulkan::ubos::model_ubo_layout(),
      vulkan::ubos::textures_ubo_layout(),
  };

  vulkan::pipelines::create(render_pass,
                            {
                                .name                   = "ui_pipeline",
                                .vertex_shader_fpath    = "vert_ui.spv",
                                .fragment_shader_fpath  = "frag_ui.spv",
                                .binding_description    = vulkan::VERTEX2D_COLOR_TEX_BINDING_DESC,
                                .attribute_descriptions = vulkan::VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC,
                                .attribute_descriptions_format_count =
                                    array::size(vulkan::VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC),
                                .ubo_layouts           = ubo_layouts,
                                .ubo_layouts_count     = array::size(ubo_layouts),
                                .disable_depth_testing = true,
                            });

  U64        mem_size    = Clay_MinMemorySize();
  Clay_Arena clay_memory = Clay_Arena{
      .capacity = mem_size,
      .memory   = arena::alloc<char>(mem_ui, mem_size),
  };

  int width, height;
  SDL_GetWindowSize(sdl_window, &width, &height);
  Clay_Initialize(clay_memory,
                  Clay_Dimensions{static_cast<F32>(width), static_cast<F32>(height)},
                  Clay_ErrorHandler{handle_clay_errors_cb});

  /* Clay_SetMeasureTextFunction(sdl_measure_text_cb, fonts); */
}

void ui::draw_frame(vulkan::CommandBufferHandle command_buffer) {
  if (!_ui_builder_fn) return;

  auto clay_commands = _ui_builder_fn();

  render::bind_pipeline(_ui_pipeline);

  int width, height;
  SDL_GetWindowSize(_sdl_window, &width, &height);

  auto proj =
      glm::ortho(0.0f, static_cast<F32>(width), static_cast<F32>(height), 0.0f, -1.0f, 1.0f);
  // proj[1][1] *= -1;

  auto view = glm::mat4(1.0f);

  render::set_view_projection(view, proj);
    // auto model = glm::translate(glm::mat4(1.0f), glm::vec3(rect.x, rect.y, 0.0f)) *
    //              glm::scale(glm::mat4(1.0f), glm::vec3(rect.w, rect.h, 1.0f));
  auto model = glm::mat4(1.0f);
  render::set_model(model);

  printf("ui:\n");
  printf("view: %s\n", glm::to_string(view).c_str());
  printf("proj: %s\n", glm::to_string(proj).c_str());
  printf("model: %s\n", glm::to_string(model).c_str());

  for (size_t i = 0; i < clay_commands.length; i++) {
    auto clay_command = Clay_RenderCommandArray_Get(&clay_commands, i);

    Rect rect = {
        .x = clay_command->boundingBox.x,
        .y = clay_command->boundingBox.y,
        .w = clay_command->boundingBox.width,
        .h = clay_command->boundingBox.height,
    };

    printf("rect: x: %f y: %f, w: %f h: %f\n", rect.x, rect.y, rect.w, rect.h);


    switch (clay_command->commandType) {
      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
        Clay_RectangleRenderData* config = &clay_command->renderData.rectangle;

        glm::vec4 color = {config->backgroundColor.r / 255.0f,
                           config->backgroundColor.g / 255.0f,
                           config->backgroundColor.b / 255.0f,
                           config->backgroundColor.a / 255.0f};

        if (config->cornerRadius.topLeft > 0) {
          printf("DRAW ROUNDED\n");
          draw_fill_rounded_rect(command_buffer, rect, config->cornerRadius.topLeft, color);
        } else {
          printf("DRAW RECT\n");
          draw_fill_rect(command_buffer, rect, color);
        }
      } break;
      case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        printf("RENDER TEXT\n");
        // Vulkan text rendering requires a separate text rendering system (e.g., Signed Distance
        // Fields or bitmap fonts)
      } break;
      case CLAY_RENDER_COMMAND_TYPE_BORDER: {
        printf("RENDER BORDER\n");
        draw_border(command_buffer, rect, &clay_command->renderData.border);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
        printf("RENDER SCISSOR START\n");
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

        auto vk_command_buffer = *vulkan::command_buffers::buffer(command_buffer);

        vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        printf("RENDER SCISSOR END\n");
        VkRect2D scissor = {{0, 0}, {UINT32_MAX, UINT32_MAX}};

        auto vk_command_buffer = *vulkan::command_buffers::buffer(command_buffer);

        vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
      } break;
      case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
        printf("RENDER IMAGE\n");
        // Vulkan texture rendering (bind image and draw quad)
      } break;
      case CLAY_RENDER_COMMAND_TYPE_NONE:
        printf("RENDER NONE\n");
        break;
      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        printf("RENDER CUSTOM\n");
        break;
    }
  }

  printf("END UI FRAME\n\n");
}

void ui::cleanup_frame() {
  for (U32 i = 0; i < _frame_meshes._size; ++i) {
    meshes::cleanup(_frame_meshes._data[i]);
  }
  array::resize(_frame_meshes, 0);
}

void ui::set_builder(UiBuilderFn ui_builder_fn) { _ui_builder_fn = ui_builder_fn; }
