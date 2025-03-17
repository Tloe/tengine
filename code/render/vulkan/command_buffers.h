#pragma once

#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace command_buffers {
    void init();
    void cleanup();

    CommandBufferHandle create();

    VkCommandBuffer* begin(CommandBufferHandle handle, VkCommandBufferUsageFlags usage_flags = 0);
    void reset(CommandBufferHandle handle);

    void submit(CommandBufferHandle handle);
    void cleanup(CommandBufferHandle handle);
    void submit_and_cleanup(CommandBufferHandle handle);

    VkCommandBuffer* buffer(CommandBufferHandle handle);
  }
}
