#include "frame.h"

#include "arena.h"
#include "defines.h"
#include "vulkan/command_buffers.h"
#include "vulkan/common.h"
#include "vulkan/handles.h"
#include "vulkan/vulkan_include.h"

namespace { Frame frames[MAX_FRAME_IN_FLIGHT]; }

namespace render::frame { Frame* current = &frames[0]; }

void render::frame::init(VkDevice logical_device) {
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (U8 i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    frames[i].index = i;

    ASSERT_SUCCESS(
        "failed to create semaphores!",
        vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &frames[i].image_available));

    ASSERT_SUCCESS(
        "failed to create semaphores!",
        vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &frames[i].render_finished));

    ASSERT_SUCCESS("failed to create semaphores!",
                   vkCreateFence(logical_device, &fence_info, nullptr, &frames[i].fence));

    frames[i].command_buffer = vulkan::command_buffers::create();

    char frame_str[7];
    snprintf(frame_str, 7, "frame%d", i);
    frames[i].arena = arena::by_name(frame_str);
  }
}

void render::frame::cleanup(VkDevice logical_device) {
  for (U8 i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    vkDestroySemaphore(logical_device, frames[i].render_finished, nullptr);
    vkDestroySemaphore(logical_device, frames[i].image_available, nullptr);
    vkDestroyFence(logical_device, frames[i].fence, nullptr);
  }
}

Frame* render::frame::next() { return &frames[(current->index + 1) % MAX_FRAME_IN_FLIGHT]; }

Frame* render::frame::previous() {
  return &frames[(current->index + MAX_FRAME_IN_FLIGHT - 1) % MAX_FRAME_IN_FLIGHT];
}
