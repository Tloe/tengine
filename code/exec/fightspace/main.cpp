#include "arena.h"
#include "clay/clay.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "engine.h"
#include "meshes.h"
#include "render.h"
#include "textures.h"
#include "ui.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/samplers.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

#include <SDL3/SDL.h>
#include <cstdint>

INIT_ARENA(render, 10000000);
INIT_ARENA(render_resources, 10000000);
INIT_ARENA(render_ui, 10000000);
INIT_ARENA(ui, 100000);
INIT_ARENA(frame0, 10000);
INIT_ARENA(frame1, 10000);

ArenaHandle mem_render_resource = arena::by_name("render_resources");

/* const int  FONT_ID_BODY_16        = 0; */
Clay_Color contentBackgroundColor = {90, 90, 90, 255};

Clay_RenderCommandArray build_ui() {
  Clay_BeginLayout();

  CLAY({
      .id = CLAY_ID("OuterContainer"),
      .layout =
          {
              .padding         = CLAY_PADDING_ALL(16),
              .childGap        = 16,
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
          },
      .backgroundColor = {43, 41, 51, 255},
  }) {
    // Child elements go inside braces
    CLAY({
        .id = CLAY_ID("HeaderBar"),
        .layout =
            {
                .sizing =
                    {
                        .width  = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_FIXED(60),
                    },
                .padding        = {16, 16, 0, 0},
                .childGap       = 16,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
            },
        .backgroundColor = contentBackgroundColor,
        .cornerRadius    = CLAY_CORNER_RADIUS(8),

    }) {
      // Header buttons go here
    }
  }

  Clay_RenderCommandArray renderCommands = Clay_EndLayout();
  for (I32 i = 0; i < renderCommands.length; i++) {
    Clay_RenderCommandArray_Get(&renderCommands, i)->boundingBox.y += 0;
  }
  return renderCommands;
}

int main() {

  VkDescriptorSetLayout ubo_layouts[] = {
      vulkan::ubos::global_ubo_layout(),
      vulkan::ubos::model_ubo_layout(),
      vulkan::ubos::textures_ubo_layout(),
  };

  vulkan::pipelines::Settings pipeline_settings[1] = {{
      .name                                = "render_pipeline",
      .vertex_shader_fpath                 = "vert.spv",
      .fragment_shader_fpath               = "frag.spv",
      .binding_description                 = vulkan::VERTEX_TEX_BINDING_DESC,
      .attribute_descriptions              = vulkan::VERTEX_TEX_ATTRIBUTE_DESC,
      .attribute_descriptions_format_count = array::size(vulkan::VERTEX_TEX_ATTRIBUTE_DESC),
      .ubo_layouts                         = ubo_layouts,
      .ubo_layouts_count                   = array::size(ubo_layouts),
  }};

  auto state = engine::init({
      .memory_init_scratch     = 1000000,
      .max_frames              = 2,
      .max_textures            = 10,
      .width                   = 1200,
      .height                  = 1024,
      .pipeline_settings       = pipeline_settings,
      .pipeline_settings_count = 1,
  });

  auto texture = textures::create_with_staging(512, 512);
  auto sampler = textures::create_sampler(1);
  textures::set_sampler(texture, sampler);

  auto vertices = array::init<vulkan::VertexTex>(mem_render_resource,
                                                 {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
                                                  {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                                                  {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                                  {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}});

  auto indices = array::init<U32>(mem_render_resource, {0, 1, 2, 2, 1, 3});

  auto mesh = meshes::create(vertices, indices);

  auto view = glm::mat4(1.0f);
  auto proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
  proj[1][1] *= -1;

  textures::set_textures(array::init<vulkan::TextureHandle>(mem_render_resource, {texture}));

  auto texture_data = array::init<U32>(mem_render_resource, 512 * 512, 512 * 512);

  auto model = glm::mat4(1.0f);

  ui::set_builder(build_ui);

  auto render_pipeline = vulkan::pipelines::by_name("render_pipeline");
  U8   r               = 255;
  while (!state->quit) {
    engine::begin_frame();
    render::bind_pipeline(render_pipeline);

    render::set_view_projection(view, proj);
    render::set_model(model);

    r++;
    U8 g = 0;
    U8 b = 0;
    U8 a = 255;
    for (U32 i = 0; i < 512; ++i) {
      for (U32 j = 0; j < 512; ++j) {
        /* r = ++r / (i * (j + 1)); */
        /* b                               = r - i * (j + 1);  */
        r %= 255;
        /* b                               = b % 150; */
        texture_data._data[i * 512 + j] = (U8(a) << 24) | (U8(b) << 16) | (U8(g) << 8) | U8(r);
      }
    }

    textures::set_data(texture, texture_data);
    render::draw_mesh(mesh);

    engine::end_frame();
  }

  textures::cleanup(sampler);
  textures::cleanup(texture);
  meshes::cleanup(mesh);

  engine::cleanup();

  return 0;
}
