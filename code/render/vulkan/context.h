#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

namespace vulkan {
  extern struct Context {
    VkInstance instance = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    VkDevice     logical_device = VK_NULL_HANDLE;
    VkQueue      graphics_queue = VK_NULL_HANDLE;
    VkQueue      present_queue  = VK_NULL_HANDLE;
    VkSurfaceKHR surface        = VK_NULL_HANDLE;

    VkSampleCountFlagBits max_msaa_samples;
  } _ctx;

  namespace context {
    void init(SDL_Window* window, bool debug);
    void cleanup(bool debug);

    VkSurfaceFormatKHR surface_format();
  }
}
