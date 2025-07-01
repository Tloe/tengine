#include "index_buffers.h"

#include "buffers.h"
#include "handles.h"
#include "vulkan_include.h"
#include <sys/select.h>

vulkan::IndexBufferHandle vulkan::index_buffers::create(const DynamicArray<U32>& indices) {
  VkDeviceSize byte_size = sizeof(indices._data[0]) * indices._size;

  auto staging =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  buffers::copy(staging, indices._data, byte_size);

  auto index_buffer =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffers::copy(index_buffer, staging, byte_size);
  buffers::cleanup(staging);

  return index_buffer;
}

vulkan::IndexBufferHandle vulkan::index_buffers::create(VkDeviceSize byte_size) {
  auto index_buffer =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  return index_buffer;
}

vulkan::IndexBufferHandle vulkan::index_buffers::create(const U32* indices, VkDeviceSize size) {
  VkDeviceSize byte_size = size * sizeof(U32);
  auto         staging =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  buffers::copy(staging, static_cast<const void*>(indices), byte_size);

  auto index_buffer =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  buffers::copy(index_buffer, staging, byte_size);
  buffers::cleanup(staging);

  return index_buffer;
}

void vulkan::index_buffers::cleanup(IndexBufferHandle handle) {
  buffers::cleanup(handle);
}
