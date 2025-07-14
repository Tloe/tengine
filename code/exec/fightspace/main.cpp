#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include "engine.h"
#include "exec/fightspace/simulation.h"
#include "exec/fightspace/ui.h"
#include "render.h"
#include "render/ui.h"
#include "vulkan/handles.h"
#include "vulkan/ssbo.h"
#include "vulkan/ubos.h"

ARENA_INIT(scratch, 10000000);
ARENA_INIT(render, 100000000);
ARENA_INIT(frame0, 100000);
ARENA_INIT(frame1, 100000);
ARENA_INIT(frame2, 100000);
ARENA_INIT(level, 100000000);

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

  simulation::init(0, 0, 1024, 1024, vulkan::ubos::mapped(material_ssbo_ubo));

  ui::set_builder(fightspace_ui);

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

  engine::cleanup();

  return 0;
}
