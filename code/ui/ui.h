#pragma once

#include "types.h"
#include <SDL3/SDL_video.h>

namespace ui {
  void init(SDL_Window* window);
  void update(F32 dt);
  void cleanup();
}
