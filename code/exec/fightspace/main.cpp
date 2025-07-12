#include "arena.h"
#include "clay/clay.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "ds_string.h"
#include "engine.h"
#include "exec/fightspace/simulation.h"
#include "meshes.h"
#include "render.h"
#include "ui.h"
#include "vulkan/handles.h"
#include "vulkan/ssbo.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

ARENA_INIT(scratch, 10000000);
ARENA_INIT(render, 100000000);
ARENA_INIT(frame0, 100000);
ARENA_INIT(frame1, 100000);
ARENA_INIT(frame2, 100000);
ARENA_INIT(level, 100000000);

Clay_RenderCommandArray build_ui() {
  Clay_BeginLayout();

  CLAY({
      .layout =
          {
              .sizing          = {.width  = CLAY_SIZING_FIXED(200),
                                  .height = CLAY_SIZING_FIXED(500)},
              .padding         = CLAY_PADDING_ALL(16),
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
          .layout          = {.sizing = {.width  = CLAY_SIZING_PERCENT(.9),
                                         .height = CLAY_SIZING_FIXED(50)}},
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
              .width        = 1920,
              .height       = 1080,
              .ui_fonts = S_DARRAY(String, {S_STRING("Roboto-Regular.ttf")}),
              .ubo_settings =
                  vulkan::ubos::Settings{
                      .ubo_count         = 2,
                      .ssbo_count        = 1,
                      .max_textures      = 4,
                      .texture_set_count = 2,
                  },
          },
  });

  auto material_ssbo = vulkan::ssbo::create(192 * 108 * sizeof(U8));

  auto texture_ubo =
      vulkan::ubos::create_texture_set(0,
                                       vulkan::StageFlags::SHADER_FRAGMENT,
                                       1);

  auto material_ssbo_ubo =
      vulkan::ubos::create_ssbo_ubo(1,
                                    vulkan::StageFlags::SHADER_FRAGMENT,
                                    material_ssbo);

  auto render_pipeline = render::create_pipeline({
      .vertex_shader_fpath   = "vert.spv",
      .fragment_shader_fpath = "frag.spv",
      .ubos = S_DARRAY(vulkan::UBOHandle, texture_ubo, material_ssbo_ubo),
  });

  simulation::init(0, 0, 1000, 1000, vulkan::ubos::mapped(material_ssbo_ubo));

  vulkan::Vertex2D vertices[] = {
      {{-1.0f, -1.0f}},
      {{3.0f, -1.0f}},
      {{-1.0f, 3.0f}},
  };

  auto mesh = meshes::create(vertices, array::size(vertices));

  meshes::set_constants(mesh, glm::mat4(1.0f), 0);

  ui::set_builder(build_ui);

  while (!state->quit) {
    engine::begin_frame();
    render::bind_pipeline(render_pipeline);

    simulation::simulate();

    render::draw();

    engine::end_frame();
  }

  vulkan::ssbo::cleanup(material_ssbo);
  vulkan::ubos::cleanup(material_ssbo_ubo);

  vulkan::ubos::cleanup(texture_ubo);

  meshes::cleanup(mesh);

  engine::cleanup();

  return 0;
}
