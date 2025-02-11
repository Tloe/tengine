#include "vulkan.h"
#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace {
   VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
  }
}

VkDebugUtilsMessengerCreateInfoEXT vulkan::debug_messenger_create_info() {
  VkDebugUtilsMessengerCreateInfoEXT create_info{};

  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;

  return create_info;
}

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

vulkan::SwapChainSupportDetails
vulkan::query_swap_chain_support(mem::Arena *perm, VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details = {
      .capabilities = VkSurfaceCapabilitiesKHR{},
      .formats = ds::array::init<VkSurfaceFormatKHR>(perm),
      .presentModes = ds::array::init<VkPresentModeKHR>(perm),
  };

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
    ds::array::resize(details.presentModes, presentModeCount);
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

VkPhysicalDeviceProperties vulkan::properties(VkPhysicalDevice physical_device) {
  VkPhysicalDeviceProperties properties;

  vkGetPhysicalDeviceProperties(physical_device, &properties);
  return properties;
}

VkSampleCountFlagBits vulkan::max_msaa_samples(VkPhysicalDevice physical_device) {
  auto props = properties(physical_device);

  VkSampleCountFlags counts =
      props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

