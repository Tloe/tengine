#pragma once

#include "render.h"
#include "types.h"
#include "vulkan/handles.h"

namespace engine {
  struct State {
    bool quit = false;
    U16  fps  = 0;
    F64  dt   = 0.0;
  };

  const State* init(const render::Settings& render_settings);
  void   cleanup();
  void   begin_frame();
  void   end_frame();
}
