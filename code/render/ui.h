#pragma once

#include "clay//clay.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include "vulkan/handles.h"

struct SDL_Window;

namespace ui {
  typedef Clay_RenderCommandArray(*UiBuilderFn)();

  void init(SDL_Window* window, vulkan::RenderPassHandle, DynamicArray<String>& fonts);

  void cleanup();

  void draw_frame();
  void cleanup_frame();

  void set_builder(UiBuilderFn ui_builder_fn);
}
