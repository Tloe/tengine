#ifndef DEVICE_H
#define DEVICE_H

#include "types.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace device {
    U32 memory_type(VkPhysicalDevice      physical_device,
                    uint32_t              type_filter,
                    VkMemoryPropertyFlags properties);

    VkFormat find_depth_format(VkPhysicalDevice physical_device);

    VkPhysicalDeviceProperties properties(VkPhysicalDevice physical_device);
  }

}
#endif
