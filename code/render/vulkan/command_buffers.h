#pragma once

#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace command_buffers {
    void init();
    void cleanup();

    CommandBufferHandle create();

    VkCommandBuffer* begin(CommandBufferHandle command_buffer, VkCommandBufferUsageFlags usage_flags = 0);
    void reset(CommandBufferHandle command_buffer);

    void submit(CommandBufferHandle command_buffer);
    void cleanup(CommandBufferHandle command_buffer);
    void submit_and_cleanup(CommandBufferHandle command_buffer);

    VkCommandBuffer* buffer(CommandBufferHandle command_buffer);
  }
}
