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

void init_simulation();
void simulate();
void add_cell(U32 x, U32 y, MaterialType type);
