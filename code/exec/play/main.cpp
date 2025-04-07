#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "engine.h"
#include "meshes.h"
#include "render.h"
#include "textures.h"
#include "types.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan/samplers.h"
#include "vulkan/swap_chain.h"
#include "vulkan/textures.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

#include <SDL3/SDL.h>

INIT_ARENA(render, 100000);
INIT_ARENA(render_resources, 100000);
INIT_ARENA(frame0, 10000);
INIT_ARENA(frame1, 10000);

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

  auto state = engine::init(render::Settings{
      .memory_init_scratch     = 1000000,
      .max_frames              = 2,
      .max_textures            = 10,
      .width                   = 1200,
      .height                  = 1024,
      .pipeline_settings       = pipeline_settings,
      .pipeline_settings_count = 1,
  });

  auto viking_texture = textures::create_mipmaped("viking_room.png");
  auto mip_levels     = *vulkan::images::mip_levels(viking_texture);
  auto sampler        = textures::create_sampler(mip_levels);
  vulkan::textures::set_sampler(viking_texture, sampler);

  auto viking_mesh = meshes::create("viking_room.obj");

  auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                          glm::vec3(0.0f, 0.0f, 0.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  auto proj = glm::perspective(glm::radians(45.0f),
                               vulkan::_swap_chain.extent.width /
                                   static_cast<float>(vulkan::_swap_chain.extent.height),
                               0.1f,
                               10.0f);
  proj[1][1] *= -1;

  textures::set_textures(
      array::init<vulkan::TextureHandle>(arena::by_name("render"), {viking_texture}));

  float rotation_angle = 0.0f;

  auto render_pipeline = vulkan::pipelines::by_name("render_pipeline");
  while (!state->quit) {
    engine::begin_frame();
    render::bind_pipeline(render_pipeline);

    render::set_view_projection(view, proj);

    rotation_angle += glm::radians(90.0f) * state->dt;
    if (rotation_angle > glm::two_pi<F32>()) {
      rotation_angle -= glm::two_pi<F32>();
    }

    auto model = glm::rotate(glm::mat4(1.0f), rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
    render::set_model(model);

    render::draw_mesh(viking_mesh);

    engine::end_frame();
  }

  textures::cleanup(sampler);
  vulkan::textures::cleanup(viking_texture);
  meshes::cleanup(viking_mesh);

  engine::cleanup();
  return 0;
}
