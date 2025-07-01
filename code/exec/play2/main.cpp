#include "arena.h"
#include "clay/clay.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "engine.h"
#include "meshes.h"
#include "render.h"
#include "textures.h"
#include "ui.h"
#include "vulkan/handles.h"
#include "vulkan/samplers.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

ARENA_INIT(scratch, 10000000);
ARENA_INIT(render, 100000000);
ARENA_INIT(ui, 100000);
ARENA_INIT(frame0, 100000);
ARENA_INIT(frame1, 100000);
ARENA_INIT(frame2, 100000);
ARENA_INIT(fightspace, 100000);
ARENA_INIT(level, 100000);

Clay_RenderCommandArray build_ui() {
  Clay_BeginLayout();

  CLAY({
      .layout =
          {
              .sizing  = {.width = CLAY_SIZING_FIXED(200), .height = CLAY_SIZING_FIXED(500)},
              .padding = CLAY_PADDING_ALL(16),
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width  = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding         = CLAY_PADDING_ALL(16),
                .childGap        = 10,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        .backgroundColor = {.r = 107, .g = 167, .b = 149, .a = 255},
        .border =
            {
                .color = {.r = 255, .g = 255, .b = 255, .a = 255},
                .width = {2, 2, 2, 2, 0},
            },
    }) {
      CLAY({
          .layout = {.sizing = {.width = CLAY_SIZING_PERCENT(.9), .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {.r = 0, .g = 0, .b = 100, .a = 255},
          .cornerRadius    = CLAY_CORNER_RADIUS(8),
          .border =
              {
                  .color = {.r = 255, .g = 189, .b = 39, .a = 255},
                  .width = {2, 2, 2, 2, 0},
              },
      }) {
        CLAY({.layout = {.sizing         = {.width = CLAY_SIZING_GROW(0)},
                         .padding        = CLAY_PADDING_ALL(16),
                         .childGap       = 16,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         }}}) {
          CLAY_TEXT(CLAY_STRING("Howdy"),
                    CLAY_TEXT_CONFIG({
                        .textColor = {255, 255, 255, 255},
                        .fontId    = 0,
                        .fontSize  = 16,
                    }));
        }
      }
      CLAY({
          .layout          = {.sizing = {.width  = CLAY_SIZING_PERCENT(0.9),
                                         .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {.r = 100, .g = 0, .b = 0, .a = 255},
          .cornerRadius    = CLAY_CORNER_RADIUS(8),
          .border =
              {
                  .color = {.r = 255, .g = 189, .b = 39, .a = 255},
                  .width = {2, 2, 2, 2, 0},
              },
      }) {
        // etc
      }
    }
  }

  Clay_RenderCommandArray clay_commands = Clay_EndLayout();
  for (I32 i = 0; i < clay_commands.length; i++) {
    Clay_RenderCommandArray_Get(&clay_commands, i)->boundingBox.y += 0;
  }
  return clay_commands;
}

int main() {
  auto state = engine::init({
      .render =
          {
              .max_frames   = 2,
              .max_textures = 10,
              .width        = 1200,
              .height       = 1024,
              .ui_fonts     = S_DARRAY(String, {S_STRING("Roboto-Regular.ttf")}),
              .ubo_settings =
                  vulkan::ubos::Settings{
                      .ubo_count         = 2,
                      .max_textures      = 4,
                      .texture_set_count = 2,
                  },
          },
  });

  auto global_ubo  = vulkan::ubos::create_ubo_buffer(0,
                                                    vulkan::StageFlags::SHADER_VERTEX,
                                                    sizeof(vulkan::GlobalUBO));
  auto texture_ubo = vulkan::ubos::create_texture_set(1, vulkan::StageFlags::SHADER_FRAGMENT, 1);

  auto render_pipeline = render::create_pipeline({
      .vertex_shader_fpath                 = "vert.spv",
      .fragment_shader_fpath               = "frag.spv",
      .binding_description                 = vulkan::VERTEX_TEX_BINDING_DESC,
      .attribute_descriptions              = vulkan::VERTEX_TEX_ATTRIBUTE_DESC,
      .attribute_descriptions_format_count = array::size(vulkan::VERTEX_TEX_ATTRIBUTE_DESC),
      .ubos                                = S_DARRAY(vulkan::UBOHandle, global_ubo, texture_ubo),
  });

  auto world_texture = textures::create_with_staging(512, 512);
  auto sampler       = textures::create_sampler(1);
  textures::set_sampler(world_texture, sampler);

  vulkan::VertexTex vertices[] = {
      {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
      {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
  };

  U32 indices[] = {0, 1, 2, 2, 1, 3};

  auto mesh = meshes::create(vertices, array::size(vertices), indices, array::size(indices));

  meshes::set_constants(mesh, glm::mat4(1.0f), 0);

  auto view = glm::mat4(1.0f);
  auto proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
  proj[1][1] *= -1;

  U32 texture_data[512 * 512];

  ui::set_builder(build_ui);

  U8 r = 255;

  while (!state->quit) {
    engine::begin_frame();
    render::bind_pipeline(render_pipeline);

    render::set_view_projection(global_ubo, view, proj);
    textures::set_textures(texture_ubo, S_DARRAY(vulkan::TextureHandle, {world_texture}));

    r += 2;
    U8 g = 0;
    U8 b = 0;
    U8 a = 255;
    for (U32 i = 0; i < 512; ++i) {
      for (U32 j = 0; j < 512; ++j) {
        // r = ++r / (i * (j + 1));
        b = (r - i * (j + 1)) % 150;
        r %= 100;
        /* b                               = b % 150; */
        texture_data[i * 512 + j] = (U8(a) << 24) | (U8(b) << 16) | (U8(g) << 8) | U8(r);
      }
    }

    textures::set_data(world_texture, texture_data, static_cast<U64>(sizeof(U32) * 512 * 512));
    render::draw(mesh);

    engine::end_frame();
  }

  vulkan::ubos::cleanup(texture_ubo);
  vulkan::ubos::cleanup(global_ubo);

  textures::cleanup(sampler);
  textures::cleanup(world_texture);

  meshes::cleanup(mesh);

  engine::cleanup();

  return 0;
}
