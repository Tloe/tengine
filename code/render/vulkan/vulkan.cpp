#include "vulkan.h"
#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include "vulkan/context.h"
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

ds::DynamicArray<const char *> vulkan::required_extension(mem::Arena *perm, bool debug) {
  U32 sdl_extension_count = 0;
  const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

  auto extensions =
      ds::array::init<const char *>(perm, sdl_extensions, sdl_extensions + sdl_extension_count);

  if (debug) {
    ds::array::push_back(extensions, static_cast<const char *>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
  }

  return extensions;
}

ds::DynamicArray<ds::String> vulkan::available_extensions(mem::Arena *perm,
                                                          VkPhysicalDevice device) {
  U32 available_extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, nullptr);

  VkExtensionProperties available_extensions[available_extension_count];
  vkEnumerateDeviceExtensionProperties(device,
                                       nullptr,
                                       &available_extension_count,
                                       available_extensions);

  auto result = ds::array::init<ds::String>(perm, available_extension_count);

  for (U32 i = 0; i < available_extension_count; i++) {
    ds::array::push_back(result, ds::string::init(perm, available_extensions[i].extensionName));
  }

  return result;
}

bool vulkan::queue_family_indices(VkPhysicalDevice device,
                                  VkSurfaceKHR surface,
                                  U32 *graphicsFamily,
                                  U32 *presentFamily) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  VkQueueFamilyProperties queue_families[queue_family_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  bool graphics_family_found, present_family_found = false;
  for (U32 i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphicsFamily = i;
      graphics_family_found = true;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support) {
      *presentFamily = i;
      present_family_found = true;
    }

    if (graphics_family_found && present_family_found) {
      return true;
    }
  }

  return false;
}

VkCommandBuffer vulkan::begin_single_time_commands(ContextPtr ctx) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = ctx->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(ctx->logical_device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

void vulkan::end_single_time_commands(ContextPtr ctx, VkCommandBuffer command_buffer) {

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(ctx->graphics_queue);

  vkFreeCommandBuffers(ctx->logical_device, ctx->command_pool, 1, &command_buffer);
}
