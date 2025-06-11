#pragma once

#include "render.h"
#include "types.h"

namespace engine {
  struct Setting {
    render::Settings     render;
  };

  struct State {
    bool quit = false;
    U16  fps  = 0;
    F64  dt   = 0.0;
  };

  const State* init(const engine::Setting& settings);

  void cleanup();
  void begin_frame();
  void end_frame();
}
