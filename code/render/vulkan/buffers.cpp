#include "buffers.h"

#include "arena.h"
#include "context.h"
#include "ds_hashmap.h"
#include "handle.h"
#include "vulkan/command_buffers.h"
#include "vulkan/device.h"
#include "vulkan/images.h"
#include "vulkan/vulkan.h"

#include <cstdio>
#include <vulkan/vulkan_core.h>

namespace {
  auto mem_render_resources = arena::by_name("render_resources");

  struct BufferData {
    U32            byte_size;
    VkBuffer       vk_buffer;
    VkDeviceMemory vk_memory;
  };

  HashMap16<BufferData> index_buffers = hashmap::init16<BufferData>(mem_render_resources);

  handles::Allocator<vulkan::BufferHandle, U16, 100> _handles;
}

vulkan::BufferHandle vulkan::buffers::create(VkDeviceSize          byte_size,
                                             VkBufferUsageFlags    usage,
                                             VkMemoryPropertyFlags properties) {
  auto buffer = handles::next(_handles);

  auto buffer_data       = hashmap::insert(index_buffers, buffer.value);
  buffer_data->byte_size = byte_size;

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size        = byte_size;
  buffer_info.usage       = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  ASSERT_SUCCESS("failed to create buffer!",
                 vkCreateBuffer(vulkan::_ctx.logical_device, &buffer_info, nullptr, &buffer_data->vk_buffer));

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(vulkan::_ctx.logical_device, buffer_data->vk_buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize  = mem_requirements.size;
  alloc_info.memoryTypeIndex = device::memory_type(vulkan::_ctx.physical_device,
                                                   mem_requirements.memoryTypeBits,
                                                   properties);

  ASSERT_SUCCESS(
      "failed to allocate buffer memory!",
      vkAllocateMemory(vulkan::_ctx.logical_device, &alloc_info, nullptr, &buffer_data->vk_memory));

  vkBindBufferMemory(vulkan::_ctx.logical_device, buffer_data->vk_buffer, buffer_data->vk_memory, 0);

  return buffer;
}

void vulkan::buffers::cleanup(BufferHandle buffer) {
  vkDestroyBuffer(vulkan::_ctx.logical_device, *vk_buffer(buffer), nullptr);
  vkFreeMemory(vulkan::_ctx.logical_device, *vk_memory(buffer), nullptr);
  hashmap::erase(index_buffers, buffer.value);

  handles::free(_handles, buffer);
}

void vulkan::buffers::copy(BufferHandle dst, const void* src, VkDeviceSize byte_size) {
  void* mapped;
  auto  buffer_memory = *buffers::vk_memory(dst);
  vkMapMemory(vulkan::_ctx.logical_device, buffer_memory, 0, byte_size, 0, &mapped);
  memcpy(mapped, src, byte_size);
  vkUnmapMemory(vulkan::_ctx.logical_device, buffer_memory);
}

void vulkan::buffers::copy(BufferHandle dst, BufferHandle src, VkDeviceSize size) {
  auto command_buffer = command_buffers::create();
  command_buffers::begin(command_buffer);

  VkBufferCopy copy_region{};
  copy_region.size = size;
  vkCmdCopyBuffer(*command_buffers::buffer(command_buffer),
                  *vk_buffer(src),
                  *vk_buffer(dst),
                  1,
                  &copy_region);

  command_buffers::submit_and_cleanup(command_buffer);
}

void vulkan::buffers::copy(ImageHandle dst, BufferHandle src, U32 w, U32 h) {
  auto cmds = command_buffers::create();
  command_buffers::begin(cmds);

  VkBufferImageCopy region{};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0;
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {w, h, 1};

  vkCmdCopyBufferToImage(*command_buffers::buffer(cmds),
                         *vk_buffer(src),
                         *images::vk_image(dst),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  command_buffers::submit_and_cleanup(cmds);
}

VkBuffer* vulkan::buffers::vk_buffer(BufferHandle buffer) {
  return &hashmap::value(index_buffers, buffer.value)->vk_buffer;
}

VkDeviceMemory* vulkan::buffers::vk_memory(BufferHandle buffer) {
  return &hashmap::value(index_buffers, buffer.value)->vk_memory;
}

U32 vulkan::buffers::byte_size(BufferHandle buffer) {
  return hashmap::value(index_buffers, buffer.value)->byte_size;
}
