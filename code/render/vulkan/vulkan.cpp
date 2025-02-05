#include "vulkan.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

ds::DynamicArray<const char *> vulkan::required_extension(mem::Arena *scratch, bool debug) {
  U32 sdl_extension_count = 0;
  const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

  auto extensions =
      ds::array::init<const char *>(scratch, sdl_extensions, sdl_extensions + sdl_extension_count);

  if (debug) {
    ds::array::push_back(extensions, static_cast<const char *>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
  }

  return extensions;
}

ds::DynamicArray<ds::String> vulkan::available_extensions(mem::Arena *scratch,
                                                          VkPhysicalDevice device) {
  U32 available_extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, nullptr);

  VkExtensionProperties available_extensions[available_extension_count];
  vkEnumerateDeviceExtensionProperties(device,
                                       nullptr,
                                       &available_extension_count,
                                       available_extensions);

  auto result = ds::array::init<ds::String>(scratch, available_extension_count);
  for (U32 i = 0; i < available_extension_count; i++) {
    ds::array::push_back(result, ds::string::init(scratch, available_extensions[i].extensionName));
  }

  return result;
}

vulkan::SwapChainSupportDetails
vulkan::query_swap_chain_support(mem::Arena *a, VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  U32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    ds::array::resize(details.formats, formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats._data);
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    ds::array::resize(details.formats, formatCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                              surface,
                                              &presentModeCount,
                                              details.presentModes._data);
  }

  return details;
}

bool vulkan::queue_family_indices(VkPhysicalDevice device,
                                  VkSurfaceKHR surface,
                                  U32 *graphicsFamily,
                                  U32 *presentFamily) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  VkQueueFamilyProperties queueFamilies[queue_family_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queueFamilies);

  bool graphics_family_found, present_family_found = false;
  for (U32 i = 0; i < queue_family_count; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphicsFamily = i;
      graphics_family_found = true;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport) {
      *presentFamily = i;
      present_family_found = true;
    }

    if (graphics_family_found && present_family_found) {
      return true;
    }
  }

  return false;
}
