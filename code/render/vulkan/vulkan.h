#pragma once

#include "ds_array_dynamic.h"

#include <cstdlib>
#include <vulkan/vulkan_core.h>

struct String;

#define ASSERT_SUCCESS(fail_msg, result)                                                           \
  ({                                                                                               \
    if (result != VK_SUCCESS) {                                                                    \
      printf("%s", fail_msg);                                                                      \
      exit(0);                                                                                     \
    }                                                                                              \
  })

namespace vulkan {
  constexpr const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  constexpr const char* device_extensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
  };

  DynamicArray<const char*> required_instance_extension(bool debug);
  DynamicArray<String>      available_extensions(VkPhysicalDevice device);

  bool queue_family_indices(VkPhysicalDevice device,
                            VkSurfaceKHR     surface,
                            U32*             graphicsFamily,
                            U32*             presentFamily);

}
