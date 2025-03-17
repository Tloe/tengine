#pragma once

#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace buffers {
    BufferHandle
         create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void cleanup(BufferHandle handle);

    void copy(BufferHandle dst, void* src, U32 byte_size);
    void copy(BufferHandle dst, BufferHandle src, VkDeviceSize size);
    void copy(ImageHandle dst, BufferHandle src, U32 w, U32 h);

    VkBuffer*       buffer(BufferHandle handle);
    VkDeviceMemory* memory(BufferHandle handle);
    U32             byte_size(BufferHandle handle);
  }
}
