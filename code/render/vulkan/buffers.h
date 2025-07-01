#pragma once

#include "handles.h"

#include "vulkan_include.h"

namespace vulkan {
  namespace buffers {
    BufferHandle
         create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void cleanup(BufferHandle buffer);

    void copy(BufferHandle dst, const void* src, VkDeviceSize byte_size);
    void copy(BufferHandle dst, BufferHandle src, VkDeviceSize size);
    void copy(ImageHandle dst, BufferHandle src, U32 w, U32 h);

    void* map(BufferHandle buffer);
    void unmap(BufferHandle buffer);

    VkBuffer*       vk_buffer(BufferHandle buffer);
    VkDeviceMemory* vk_memory(BufferHandle buffer);
    U32             byte_size(BufferHandle buffer);
  }
}
