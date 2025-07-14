#pragma once

#include "types.h"

enum class MaterialType : U8 {
  BORDER,
  AIR,
  SAND,
  WATER,
  WOOD,
  STONE,
  FIRE,
};

namespace simulation {
  void init(U32 view_x,
            U32 view_y,
            U32 level_width,
            U32 level_height,
            U8* gpu_memory);
  void simulate();
  void set_view(U32 x, U32 y);
  void add_cell(U32 x, U32 y, MaterialType type);
}
