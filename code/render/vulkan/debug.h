#pragma once

#include <vulkan/vulkan_core.h>

namespace vulkan::debug {
  void init(VkInstance instance);
  void cleanup(VkInstance instance);

  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();
}

