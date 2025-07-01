#pragma once

#include "handles.h"
#include "vulkan_include.h"

namespace vulkan {
  namespace command_buffers {
    void init();
    void cleanup();

    CommandBufferHandle create();

    VkCommandBuffer* begin(CommandBufferHandle command_buffer, VkCommandBufferUsageFlags usage_flags = 0);
    void reset(CommandBufferHandle command_buffer);

    void submit(CommandBufferHandle command_buffer);

    void wait(CommandBufferHandle handle);

    VkCommandBuffer* buffer(CommandBufferHandle command_buffer);
  }
}
