#pragma once

#include "defines.h"
#include "memory/handles.h"
#include "types.h"
#include "vulkan/handles.h"
#include "vulkan/vulkan_include.h"

struct Frame {
  U8                          index;
  U32                         vk_image_index;
  VkSemaphore                 image_available;
  VkSemaphore                 render_finished;
  VkFence                     fence;
  vulkan::CommandBufferHandle command_buffer;
  vulkan::PipelineHandle      current_pipeline;
  ArenaHandle                 arena;
};

namespace render::frame {
  extern Frame* current;

  void init(VkDevice logical_device);
  void cleanup(VkDevice logical_device);

  Frame* next();
  Frame* previous();
}
