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

ARENA_INIT(scratch, 100000);
ARENA_INIT(render, 10000000);
ARENA_INIT(render_resources, 10000000);
ARENA_INIT(render_ui, 10000000);
ARENA_INIT(ui, 100000);
ARENA_INIT(frame0, 100000);
ARENA_INIT(frame1, 100000);
ARENA_INIT(play, 100000);

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
                      .ubo_count         = 4,
                      .max_textures      = 4,
                      .texture_set_count = 2,
                  },
          },
  });

  auto global_ubo  = vulkan::ubos::create_ubo(0, sizeof(vulkan::GlobalUBO));
  auto texture_ubo = vulkan::ubos::create_texture_set(2, 1);

  auto render_pipeline = render::create_pipeline({
      .vertex_shader_fpath                 = "vert.spv",
      .fragment_shader_fpath               = "frag.spv",
      .binding_description                 = vulkan::VERTEX_TEX_BINDING_DESC,
      .attribute_descriptions              = vulkan::VERTEX_TEX_ATTRIBUTE_DESC,
      .attribute_descriptions_format_count = array::size(vulkan::VERTEX_TEX_ATTRIBUTE_DESC),
      .ubos                                = S_DARRAY(vulkan::UBOHandle, global_ubo, texture_ubo),
  });

  auto viking_texture = textures::load_mipmaped("viking_room.png");
  auto mip_levels     = vulkan::images::mip_levels(viking_texture);
  auto sampler        = textures::create_sampler(mip_levels);
  vulkan::textures::set_sampler(viking_texture, sampler);

  auto viking_mesh = meshes::create(arena::by_name("play"), "viking_room.obj");

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
      texture_ubo,
      A_DARRAY(vulkan::TextureHandle, arena::by_name("render"), {viking_texture}));

  float rotation_angle = 0.0f;

  while (!state->quit) {
    engine::begin_frame();
    render::bind_pipeline(render_pipeline);

    render::set_view_projection(global_ubo, view, proj);

    rotation_angle += glm::radians(90.0f) * state->dt;
    if (rotation_angle > glm::two_pi<F32>()) {
      rotation_angle -= glm::two_pi<F32>();
    }

    auto model = glm::rotate(glm::mat4(1.0f), rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
    meshes::set_constants(viking_mesh, model, 0);

    render::draw(viking_mesh);

    engine::end_frame();
  }

  textures::cleanup(sampler);
  vulkan::textures::cleanup(viking_texture);
  meshes::cleanup(viking_mesh);

  engine::cleanup();
  return 0;
}
