#include "device.h"

#include "ds_array_static.h"

#include <cstdio>
#include <cstdlib>

U32 vulkan::device::memory_type(VkPhysicalDevice      physical_device,
                                uint32_t              typeFilter,
                                VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  for (U32 i = 0; i < memory_properties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  printf("failed to find suitable memory type!");
  exit(0);

  return 0;
}

VkFormat vulkan::device::find_depth_format(VkPhysicalDevice physical_device) {
  VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT,
                           VK_FORMAT_D32_SFLOAT_S8_UINT,
                           VK_FORMAT_D24_UNORM_S8_UINT};

  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

  for (U32 i = 0; i < array::size(candidates); i++) {
    VkFormatProperties device_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &device_properties);

    if (tiling == VK_IMAGE_TILING_LINEAR && (device_properties.linearTilingFeatures & features) == features) {
      return candidates[i];
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (device_properties.optimalTilingFeatures & features) == features) {
      return candidates[i];
    }
  }

  printf("failed to find supported format!");
  exit(0);
  return VkFormat{};
}

VkPhysicalDeviceProperties vulkan::device::properties(VkPhysicalDevice physical_device) {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device, &properties);

  return properties;
}
