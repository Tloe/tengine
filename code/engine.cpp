#include "engine.h"

#include "render.h"
#include "types.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

namespace {
  engine::State state;
  U64           previous_time = 0;
  U64           current_time  = 0;
  U64           frame_count   = 0;
  U64           fps_last_time;

  render::Renderer renderer;
}

engine::State& engine::init(const render::Settings& render_settings) {
  previous_time = fps_last_time = SDL_GetPerformanceCounter();

  renderer = render::init(render_settings);

  return state;
}

void engine::cleanup() { render::cleanup(renderer); }

void engine::begin_frame() {
  previous_time = current_time;
  current_time  = SDL_GetPerformanceCounter();
  state.dt      = static_cast<F64>(current_time - previous_time) / SDL_GetPerformanceFrequency();

  ++frame_count;

  SDL_Event e;
  SDL_zero(e);
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_EVENT_QUIT: {
        state.quit = true;
        break;
      }
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        render::resize_framebuffers();
        break;
      }
    }
  }

  render::begin_frame(renderer);
}

void engine::end_frame() {
  render::end_frame(renderer);

  if ((current_time - fps_last_time) > SDL_GetPerformanceFrequency()) {
    state.fps = frame_count /
                (static_cast<F64>(current_time - fps_last_time) / SDL_GetPerformanceFrequency());
    fps_last_time = current_time;
    frame_count   = 0;
  }
}
