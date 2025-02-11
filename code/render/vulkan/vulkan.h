#ifndef VULKAN_H
#define VULKAN_H

#include "ds_array_dynamic.h"
#include <vulkan/vulkan_core.h>

namespace ds {
  struct String;
}

namespace vulkan {
  constexpr const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  constexpr const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities = {};
    ds::DynamicArray<VkSurfaceFormatKHR> formats;
    ds::DynamicArray<VkPresentModeKHR> presentModes;
  };
  SwapChainSupportDetails
  query_swap_chain_support(mem::Arena *perm, VkPhysicalDevice device, VkSurfaceKHR surface);

  ds::DynamicArray<const char *> required_extension(mem::Arena *perm, bool debug);
  ds::DynamicArray<ds::String> available_extensions(mem::Arena *perm, VkPhysicalDevice device);

  bool queue_family_indices(VkPhysicalDevice device,
                            VkSurfaceKHR surface,
                            U32 *graphicsFamily,
                            U32 *presentFamily);

  VkPhysicalDeviceProperties properties(VkPhysicalDevice physical_device);
  VkSampleCountFlagBits max_msaa_samples(VkPhysicalDevice physical_device);
}

#endif
