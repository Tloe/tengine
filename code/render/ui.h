#pragma once

#include "clay//clay.h"
#include "vulkan/handles.h"

#include <SDL3/SDL_video.h>

namespace ui {
  typedef Clay_RenderCommandArray(*UiBuilderFn)();

  void init(SDL_Window* window, vulkan::RenderPassHandle);

  void draw_frame(vulkan::CommandBufferHandle command_buffer);
  void cleanup_frame();

  void set_builder(UiBuilderFn ui_builder_fn);
}
