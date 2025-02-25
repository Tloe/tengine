#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"
#include <SDL3/SDL_video.h>

struct SDL_Window; 

namespace vulkan {
  namespace window {
    SDL_Window* init(U16 w, U16 h);
    void cleanup(SDL_Window* window);
  }
}

#endif
