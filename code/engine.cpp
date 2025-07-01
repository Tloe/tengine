#include "engine.h"

#include "render.h"
#include "types.h"

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

  printf("TENGINE - ENGINE INITIALIZED\n");

  return &state;
}

void engine::cleanup() {
  printf("TENGINE - ENGINE CLEANUP\n");
  render::cleanup();

  SDL_DestroyWindow(sdl_window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  SDL_Quit();
}

void engine::begin_frame() {
  previous_time = current_time;
  current_time  = SDL_GetPerformanceCounter();
  state.dt      = static_cast<F64>(current_time - previous_time) / SDL_GetPerformanceFrequency();

  ++frame_count;

  SDL_Event event;
  SDL_zero(event);
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT: {
        state.quit = true;
        break;
      }
      case SDL_EVENT_KEY_DOWN:
        SDL_Log("Key Down: %s", SDL_GetKeyName(event.key.));
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = false;
        }
        break;

      case SDL_EVENT_KEY_UP:
        SDL_Log("Key Up: %s", SDL_GetKeyName(event.key.keysym.sym));
        break;

      // Mouse motion
      case SDL_EVENT_MOUSE_MOTION:
        SDL_Log("Mouse moved to (%d, %d)", event.motion.x, event.motion.y);
        break;

      // Mouse button press
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        SDL_Log("Mouse button %d down at (%f, %f)",
                event.button.button,
                event.button.x,
                event.button.y,
                event.button.repeat);
        break;

      // Mouse button release
      case SDL_EVENT_MOUSE_BUTTON_UP:
        SDL_Log("Mouse button %d up at (%f, %f)",
                event.button.button,
                event.button.x,
                event.button.y);
        break;

      // Mouse wheel scroll
      case SDL_EVENT_MOUSE_WHEEL:
        SDL_Log("Mouse wheel: (%d, %d)", event.wheel.x, event.wheel.y);
        break;
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

  if ((current_time - fps_last_time) > SDL_GetPerformanceFrequency()) {
    state.fps = frame_count /
                (static_cast<F64>(current_time - fps_last_time) / SDL_GetPerformanceFrequency());
    fps_last_time = current_time;
    frame_count   = 0;
  }
}
