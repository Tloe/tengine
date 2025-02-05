#ifndef VULKAN_H
#define VULKAN_H

#include "context.h"
#include "ds_array_dynamic.h"
#include <vulkan/vulkan_core.h>

namespace ds {
  struct String;
}

namespace vulkan {
  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    ds::DynamicArray<VkSurfaceFormatKHR> formats;
    ds::DynamicArray<VkPresentModeKHR> presentModes;
  };

  ds::DynamicArray<const char *> required_extension(mem::Arena *scratch, bool debug);
  ds::DynamicArray<ds::String> available_extensions(mem::Arena *scratch, VkPhysicalDevice device);
  SwapChainSupportDetails
  query_swap_chain_support(mem::Arena *scratch, VkPhysicalDevice device, VkSurfaceKHR surface);

  bool queue_family_indices(VkPhysicalDevice device,
                            VkSurfaceKHR surface,
                            U32 *graphicsFamily,
                            U32 *presentFamily);
}

#endif
