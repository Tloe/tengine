#include "command_buffers.h"

#include "context.h"
#include "ds_hashmap.h"
#include "handles.h"
#include "types.h"
#include "vulkan.h"

#include <vulkan/vulkan_core.h>

namespace {
  ArenaHandle                mem_render      = arena::by_name("render");
  HashMap16<VkCommandBuffer> command_buffers = hashmap::init16<VkCommandBuffer>(mem_render);

  VkCommandPool command_pool      = VK_NULL_HANDLE;
  U16           next_handle_value = 0;
}

void vulkan::command_buffers::init() {
  U32 graphics_family, present_family;
  vulkan::queue_family_indices(_ctx.physical_device,
                               _ctx.surface,
                               &graphics_family,
                               &present_family);

  VkCommandPoolCreateInfo pool_create_info{};
  pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_create_info.queueFamilyIndex = graphics_family;

  ASSERT_SUCCESS(
      "failed to create command pool!",
      vkCreateCommandPool(vulkan::_ctx.logical_device, &pool_create_info, nullptr, &command_pool));
}

void vulkan::command_buffers::cleanup() {
  vkDestroyCommandPool(vulkan::_ctx.logical_device, command_pool, nullptr);
}

vulkan::CommandBufferHandle vulkan::command_buffers::create() {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool        = command_pool;
  alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  CommandBufferHandle handle{.value = next_handle_value++};
  auto command_buffer = hashmap::insert(::command_buffers, handle.value, VkCommandBuffer{});

  ASSERT_SUCCESS(
      "failed to allocate command buffers!",
      vkAllocateCommandBuffers(vulkan::_ctx.logical_device, &alloc_info, command_buffer));

  return handle;
}

VkCommandBuffer*
vulkan::command_buffers::begin(CommandBufferHandle handle, VkCommandBufferUsageFlags usage_flags) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = usage_flags;

  ASSERT_SUCCESS(
      "failed to begin command buffers!",
      vkBeginCommandBuffer(*hashmap::value(::command_buffers, handle.value), &begin_info));

  return hashmap::value(::command_buffers, handle.value);
}

void vulkan::command_buffers::reset(CommandBufferHandle handle) {
  vkResetCommandBuffer(*hashmap::value(::command_buffers, handle.value), 0);
}

void vulkan::command_buffers::submit(CommandBufferHandle handle) {
  auto buffer = hashmap::value(::command_buffers, handle.value);

  vkEndCommandBuffer(*buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = buffer;

  vkQueueSubmit(vulkan::_ctx.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(vulkan::_ctx.graphics_queue);
}

void vulkan::command_buffers::cleanup(CommandBufferHandle handle) {
  vkFreeCommandBuffers(vulkan::_ctx.logical_device,
                       command_pool,
                       1,
                       hashmap::value(::command_buffers, handle.value));
}

void vulkan::command_buffers::submit_and_cleanup(vulkan::CommandBufferHandle handle) {
  submit(handle);
  cleanup(handle);
}

VkCommandBuffer* vulkan::command_buffers::buffer(CommandBufferHandle handle) {
  return hashmap::value(::command_buffers, handle.value);
}
