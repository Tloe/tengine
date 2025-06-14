#include "engine.h"

#include "arena.h"
#include "fonts.h"
#include "render.h"
#include "types.h"
#include "ui.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstdlib>

namespace {
  engine::State state;
  U64           previous_time = 0;
  U64           current_time  = 0;
  U64           frame_count   = 0;
  U64           fps_last_time;

  SDL_Window* sdl_window;
}

const engine::State* engine::init(const engine::Setting& settings) {
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
  SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    printf("failed to init SDL3: %s", SDL_GetError());
    exit(0);
  }

  sdl_window =
      SDL_CreateWindow("tengine", settings.render.width, settings.render.height, SDL_WINDOW_VULKAN);

  if (!sdl_window) {
    printf("failed to create SDL3 window: %s", SDL_GetError());
    exit(0);
  }

  SDL_SetWindowResizable(sdl_window, true);

  previous_time = fps_last_time = SDL_GetPerformanceCounter();

  render::init(settings.render, sdl_window);

  return &state;
}

void engine::cleanup() {
  render::cleanup();

  ui::cleanup_frame();

  SDL_DestroyWindow(sdl_window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  SDL_Quit();
}

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

  render::begin_frame();
}

void engine::end_frame() {
  render::end_frame();

  arena::next_frame();

  if ((current_time - fps_last_time) > SDL_GetPerformanceFrequency()) {
    state.fps = frame_count /
                (static_cast<F64>(current_time - fps_last_time) / SDL_GetPerformanceFrequency());
    fps_last_time = current_time;
    frame_count   = 0;
  }
}
