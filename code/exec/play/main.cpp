#include "arena.h"
#include "ds_array_dynamic.h"
#include "mesh.h"
#include "engine.h"
#include "render.h"
#include "types.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan/samplers.h"
#include "vulkan/swap_chain.h"
#include "vulkan/textures.h"
#include "vulkan/ubos.h"

#include <SDL3/SDL.h>

G_INIT_ARENA(render, 100000);
G_INIT_ARENA(render_resources, 100000);

int main() {
  INIT_ARENA(frame0, 10000);
  INIT_ARENA(frame1, 10000);

  auto state = engine::init(render::Settings{
      .memory_init_scratch = 1000000,
      .max_frames          = 2,
      .max_textures        = 10,
      .width               = 1200,
      .height              = 1024,
  });

  auto viking_texture = vulkan::textures::create_mipmaped("viking_room.png");
  auto mip_levels     = *vulkan::images::mip_levels(viking_texture);
  auto sampler        = vulkan::texture_samplers::create(mip_levels);
  vulkan::textures::set_sampler(viking_texture, sampler);

  auto viking_mesh = render::meshes::create("viking_room.obj");

  auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                          glm::vec3(0.0f, 0.0f, 0.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  auto proj = glm::perspective(glm::radians(45.0f),
                               vulkan::_swap_chain.extent.width /
                                   static_cast<float>(vulkan::_swap_chain.extent.height),
                               0.1f,
                               10.0f);
  proj[1][1] *= -1;

  // temp
  vulkan::ubos::set_textures(
      array::init<vulkan::TextureHandle>(arena::by_name("render"), {viking_texture}));


  float rotation_angle = 0.0f;

  while (!state.quit) {
    engine::begin_frame();

    render::begin_frame(renderer);

    render::set_view_projection(view, proj);

    rotation_angle += glm::radians(90.0f) * state.dt;
    if (rotation_angle > glm::two_pi<F32>()) {
      rotation_angle -= glm::two_pi<F32>();
    }

    auto model = glm::rotate(glm::mat4(1.0f), rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
    render::set_model(model);

    render::draw_mesh(viking_mesh);
    render::end_frame(renderer);

    engine::begin_frame();
  }

  vulkan::texture_samplers::cleanup(sampler);
  vulkan::textures::cleanup(viking_texture);
  render::meshes::cleanup(viking_mesh);

  render::cleanup(renderer);

  return 0;
}
