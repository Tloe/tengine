#pragma once

#include "vulkan_include.h"

namespace vulkan::debug {
  void init(VkInstance instance);
  void cleanup(VkInstance instance);

  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();
}
