#ifndef DEBUG_H
#define DEBUG_H

#include <vulkan/vulkan_core.h>

namespace vulkan::debug {
  void init(VkInstance instance);
  void cleanup(VkInstance instance);

  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();
}

#endif
