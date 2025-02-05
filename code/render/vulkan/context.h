#ifndef DEBUG_H
#define DEBUG_H

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

namespace mem {
  struct Arena;
}

namespace vulkan {
  struct Context {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkSurfaceKHR surface;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  };

  namespace context {
    Context create(mem::Arena *a, SDL_Window *window, bool debug);
    void cleanup(Context in);
  }
}

#endif
