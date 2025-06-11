#include "vulkan.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_string.h"

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

DynamicArray<const char*> vulkan::required_instance_extension(bool debug) {
  U32                sdl_extension_count = 0;
  const char* const* sdl_extensions      = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);

  auto extensions = array::init<const char*>(arena::scratch(),
                                             sdl_extensions,
                                             sdl_extensions + sdl_extension_count);

  array::push_back(
      extensions,
      static_cast<const char*>(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME));

  if (debug) {
    array::push_back(extensions, static_cast<const char*>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
  }

  return extensions;
}

DynamicArray<String> vulkan::available_extensions(VkPhysicalDevice device) {
  U32 available_extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &available_extension_count, nullptr);

  VkExtensionProperties available_extensions[available_extension_count];
  vkEnumerateDeviceExtensionProperties(device,
                                       nullptr,
                                       &available_extension_count,
                                       available_extensions);

  auto result = S_DARRAY_CAP(String, available_extension_count);

  for (U32 i = 0; i < available_extension_count; i++) {
    array::push_back(result, S_STRING(available_extensions[i].extensionName));
  }

  return result;
}

bool vulkan::queue_family_indices(VkPhysicalDevice device,
                                  VkSurfaceKHR     surface,
                                  U32*             graphics_family,
                                  U32*             present_family) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  VkQueueFamilyProperties queue_families[queue_family_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  bool graphics_family_found, present_family_found = false;
  for (U32 i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      *graphics_family      = i;
      graphics_family_found = true;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support) {
      *present_family      = i;
      present_family_found = true;
    }

    if (graphics_family_found && present_family_found) {
      return true;
    }
  }

  return false;
}
