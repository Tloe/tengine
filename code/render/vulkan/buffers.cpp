#include "buffers.h"

#include "arena.h"
#include "context.h"
#include "ds_hashmap.h"
#include "vulkan/command_buffers.h"
#include "vulkan/device.h"
#include "vulkan/images.h"
#include "vulkan/vulkan.h"

namespace {
  auto                      mem_render_resources = arena::by_name("render_resources");
  HashMap16<U32>            byte_sizes           = hashmap::init16<U32>(mem_render_resources);
  HashMap16<VkBuffer>       vk_buffers           = hashmap::init16<VkBuffer>(mem_render_resources);
  HashMap16<VkDeviceMemory> vk_memory = hashmap::init16<VkDeviceMemory>(mem_render_resources);

  U16 next_handle_value = 0;
}

vulkan::BufferHandle vulkan::buffers::create(VkDeviceSize          size,
                                             VkBufferUsageFlags    usage,
                                             VkMemoryPropertyFlags properties) {
  BufferHandle handle{.value = next_handle_value++};

  hashmap::insert(byte_sizes, handle.value, static_cast<U32>(size));
  auto buffer = hashmap::insert(vk_buffers, handle.value, VkBuffer{});
  auto memory = hashmap::insert(vk_memory, handle.value, VkDeviceMemory{});

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size        = size;
  buffer_info.usage       = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  ASSERT_SUCCESS("failed to create buffer!",
                 vkCreateBuffer(vulkan::_ctx.logical_device, &buffer_info, nullptr, buffer));

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(vulkan::_ctx.logical_device, *buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize  = mem_requirements.size;
  alloc_info.memoryTypeIndex = device::memory_type(vulkan::_ctx.physical_device,
                                                   mem_requirements.memoryTypeBits,
                                                   properties);

  ASSERT_SUCCESS("failed to allocate buffer memory!",
                 vkAllocateMemory(vulkan::_ctx.logical_device, &alloc_info, nullptr, memory));

  vkBindBufferMemory(vulkan::_ctx.logical_device, *buffer, *memory, 0);

  return handle;
}

void vulkan::buffers::cleanup(BufferHandle handle) {
  vkDestroyBuffer(vulkan::_ctx.logical_device, *buffer(handle), nullptr);
  vkFreeMemory(vulkan::_ctx.logical_device, *memory(handle), nullptr);
}

void vulkan::buffers::copy(BufferHandle dst, void* src, U32 byte_size) {
  void* mapped;
  auto  buffer_memory = *buffers::memory(dst);
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
                  *buffer(src),
                  *buffer(dst),
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
                         *buffer(src),
                         *images::image(dst),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  command_buffers::submit_and_cleanup(cmds);
}

VkBuffer* vulkan::buffers::buffer(BufferHandle handle) {
  return hashmap::value(vk_buffers, handle.value);
}

VkDeviceMemory* vulkan::buffers::memory(BufferHandle handle) {
  return hashmap::value(vk_memory, handle.value);
}

U32 vulkan::buffers::byte_size(BufferHandle handle) {
  return *hashmap::value(byte_sizes, handle.value);
}
