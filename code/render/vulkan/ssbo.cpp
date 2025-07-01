#include "ssbo.h"

#include "vulkan/buffers.h"
#include "vulkan/handles.h"

vulkan::SSBOBufferHandle vulkan::ssbo::create(VkDeviceSize byte_size) {
  auto ssbo =
      buffers::create(byte_size,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  return ssbo;
}

void vulkan::ssbo::cleanup(vulkan::SSBOBufferHandle ssbo) {
  buffers::cleanup(ssbo);
}
