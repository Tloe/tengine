#include "window.h"
#include "types.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <cstdio>
#include <cstdlib>

SDL_Window *vulkan::window::init(U16 w, U16 h) {
  SDL_Window *window;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    printf("failed to init SDL3: %s", SDL_GetError());
    exit(0);
  }

  if (window = SDL_CreateWindow("SDL3 Tutorial: Hello SDL3", w, h, SDL_WINDOW_VULKAN);
      window == nullptr) {
    printf("failed to create SDL3 window: %s", SDL_GetError());
    exit(0);
  }
  return window;
}

void vulkan::window::cleanup(SDL_Window* window) {
  SDL_DestroyWindow(window);
}
