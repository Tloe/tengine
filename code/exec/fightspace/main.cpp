#include "arena.h"
#include "ds_array_dynamic.h"
#include "mesh.h"
#include "render.h"
#include "vulkan/handles.h"
#include "vulkan/samplers.h"
#include "vulkan/textures.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"

#include <SDL3/SDL.h>
#include <cstdint>

G_INIT_ARENA(render, 10000000);
G_INIT_ARENA(render_resources, 100000);

ArenaHandle mem_render_resource = arena::by_name("render_resources");

int main() {
  INIT_ARENA(frame0, 10000);
  INIT_ARENA(frame1, 10000);

  auto renderer = render::init(render::Settings{
      .memory_init_scratch = 1000000,
      .max_frames          = 2,
      .max_textures        = 10,
      .width               = 1200,
      .height              = 1024,
  });

  auto texture = vulkan::textures::create_with_staging(512, 512);
  auto sampler = vulkan::texture_samplers::create(1);
  vulkan::textures::set_sampler(texture, sampler);

  auto vertices = array::init<vulkan::Vertex>(mem_render_resource,
                                              {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
                                               {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                                               {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                               {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}});
  auto indices  = array::init<U32>(mem_render_resource, {0, 1, 2, 2, 1, 3});

  auto mesh = render::meshes::create(vertices, indices);

  auto view = glm::mat4(1.0f);
  auto proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
  proj[1][1] *= -1;

  // temp
  auto textures = array::init<vulkan::TextureHandle>(arena::by_name("render"));
  array::push_back(textures, texture);
  vulkan::ubos::set_textures(textures);

  bool quit = false;

  const U8  FPS         = 60;
  const U16 frame_delay = 1000 / FPS;
  U32       frame_start;
  U32       frame_time;

  auto texture_data = array::init<U32>(arena::by_name("render"), 512 * 512, 512 * 512);

  printf("texture_data._size %d", texture_data._size);

  U8 r = 1;
  while (!quit) {
    r++;
    r    = r % 255;
    U8 g = 1;
    U8 b = 1;
    U8 a = 255;
    for (U32 i = 0; i < 512; ++i) {
      for (U32 j = 0; j < 512; ++j) {
        r                               = r / (j+1)  + j;
        b                               = r - i / (j + 1);
        r                               = r % 255;
        b                               = b % 150;
        texture_data._data[i * 512 + j] = (U8(a) << 24) | (U8(b) << 16) | (U8(g) << 8) | U8(r);
      }
    }

    frame_start = SDL_GetTicks();

    quit = render::check_events(renderer);

    vulkan::textures::update_texture(texture, texture_data);

    render::begin_frame(renderer);

    render::set_view_projection(view, proj);

    auto model = glm::mat4(1.0f);
    render::set_model(model);

    render::draw_mesh(mesh);
    render::end_frame(renderer);

    frame_time = SDL_GetTicks() - frame_start;
    if (frame_delay > frame_time) {
      SDL_Delay(frame_delay - frame_time);
    }
  }

  vulkan::texture_samplers::cleanup(sampler);
  vulkan::textures::cleanup(texture);
  render::meshes::cleanup(mesh);

  render::cleanup(renderer);

  return 0;
}
