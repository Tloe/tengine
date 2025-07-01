#include "command_buffers.h"

#include "common.h"
#include "context.h"
#include "ds_array_static.h"
#include "handles.h"
#include "types.h"
#include "vulkan_include.h"

#include <cstdio>
#include <vulkan/vulkan_core.h>

namespace {
  constexpr U8 MAX_COMMAND_BUFFERS = 20;

  ArenaHandle mem_render = arena::by_name("render");

  VkCommandPool command_pool = VK_NULL_HANDLE;

  struct CommandBuffer {
    VkCommandBuffer vk_command_buffer;
    VkFence         vk_fence = VK_NULL_HANDLE;
    bool            recording   = false;
  };

  auto _command_buffers = array::init<CommandBuffer, MAX_COMMAND_BUFFERS>(mem_render);

  U8 next_handle = 0;
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

  for (U32 i = 0; i < _command_buffers.size; ++i) {
    _command_buffers.data[i].vk_command_buffer = VK_NULL_HANDLE;
  }
}

void vulkan::command_buffers::cleanup() {
  for (U32 i = 0; i < _command_buffers.size; ++i) {
    if (_command_buffers.data[i].vk_command_buffer != VK_NULL_HANDLE) {
      vkFreeCommandBuffers(vulkan::_ctx.logical_device,
                           command_pool,
                           1,
                           &_command_buffers.data[i].vk_command_buffer);
    }
    if (_command_buffers.data[i].vk_fence != VK_NULL_HANDLE) {
      vkDestroyFence(vulkan::_ctx.logical_device, _command_buffers.data[i].vk_fence, nullptr);
    }
  }
  vkDestroyCommandPool(vulkan::_ctx.logical_device, command_pool, nullptr);
}

vulkan::CommandBufferHandle vulkan::command_buffers::create() {
  CommandBuffer* command_buffer;

  U8 loops = 0;
  for (;;) {
    if (loops >= MAX_COMMAND_BUFFERS) {
      printf("failed to allocate command buffers, none available!\n");
      exit(0);
    }

    if (next_handle >= MAX_COMMAND_BUFFERS) next_handle = 0;

    command_buffer = &_command_buffers.data[next_handle];

    if (command_buffer->vk_command_buffer == VK_NULL_HANDLE) {
      VkCommandBufferAllocateInfo alloc_info{};
      alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      alloc_info.commandPool        = command_pool;
      alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      alloc_info.commandBufferCount = 1;
      ASSERT_SUCCESS("failed to allocate command buffers!",
                     vkAllocateCommandBuffers(vulkan::_ctx.logical_device,
                                              &alloc_info,
                                              &command_buffer->vk_command_buffer));

      VkFenceCreateInfo fence_info{};
      fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      ASSERT_SUCCESS("failed to create fence!",
                     vkCreateFence(vulkan::_ctx.logical_device,
                                   &fence_info,
                                   nullptr,
                                   &command_buffer->vk_fence));

      command_buffer->recording = true;
      break;
    } else if (!_command_buffers.data[next_handle].recording && vkGetFenceStatus(vulkan::_ctx.logical_device,
                                _command_buffers.data[next_handle].vk_fence) == VK_SUCCESS) {
      ASSERT_SUCCESS("failed to reset command buffer!",
                     vkResetCommandBuffer(command_buffer->vk_command_buffer, 0));

      command_buffer->recording = true;
      break;
    }

    next_handle++;
    loops++;
  }

  return CommandBufferHandle{.value = next_handle++};
}

VkCommandBuffer*
vulkan::command_buffers::begin(CommandBufferHandle handle, VkCommandBufferUsageFlags usage_flags) {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = usage_flags;

  auto vk_command_buffer = buffer(handle);

  ASSERT_SUCCESS("failed to begin command buffers!",
                 vkBeginCommandBuffer(*vk_command_buffer, &begin_info));

  return vk_command_buffer;
}

void vulkan::command_buffers::reset(CommandBufferHandle handle) {
  vkWaitForFences(vulkan::_ctx.logical_device,
                  1,
                  &_command_buffers.data[handle.value].vk_fence,
                  VK_TRUE,
                  UINT64_MAX);
  vkResetCommandBuffer(_command_buffers.data[handle.value].vk_command_buffer, 0);
}

void vulkan::command_buffers::submit(CommandBufferHandle handle) {
  auto command_buffer = _command_buffers.data[handle.value];

  vkEndCommandBuffer(command_buffer.vk_command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &command_buffer.vk_command_buffer;

  ASSERT_SUCCESS("failed to reset fence!",
                 vkResetFences(vulkan::_ctx.logical_device, 1, &command_buffer.vk_fence));

  _command_buffers.data[handle.value].recording = false;
  vkQueueSubmit(vulkan::_ctx.graphics_queue,
                1,
                &submit_info,
                _command_buffers.data[handle.value].vk_fence);
}

void vulkan::command_buffers::wait(CommandBufferHandle command_buffer) {
  vkWaitForFences(vulkan::_ctx.logical_device,
                  1,
                  &_command_buffers.data[command_buffer.value].vk_fence,
                  VK_TRUE,
                  UINT64_MAX);
}

VkCommandBuffer* vulkan::command_buffers::buffer(CommandBufferHandle command_buffer) {
  return &_command_buffers.data[command_buffer.value].vk_command_buffer;
}
