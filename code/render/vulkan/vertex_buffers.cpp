#include "vertex_buffers.h"

#include "vulkan/buffers.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

vulkan::VertexBufferHandle vulkan::vertex_buffers::create(const void* vertices, VkDeviceSize byte_size) {
  auto staging =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  buffers::copy(staging, vertices, byte_size);

  auto vertex_buffer =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffers::copy(vertex_buffer, staging, byte_size);

  buffers::cleanup(staging);

  return VertexBufferHandle{.value = vertex_buffer.value};
}

void vulkan::vertex_buffers::cleanup(VertexBufferHandle handle) {
  buffers::cleanup(handle);
}

