#ifndef CONTEXT_H
#define CONTEXT_H

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

namespace mem { struct Arena; }

namespace vulkan {
  struct Context {
    VkInstance instance = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    VkDevice     logical_device = VK_NULL_HANDLE;
    VkQueue      graphics_queue = VK_NULL_HANDLE;
    VkQueue      present_queue  = VK_NULL_HANDLE;
    VkSurfaceKHR surface        = VK_NULL_HANDLE;

    VkCommandPool command_pool = VK_NULL_HANDLE;

    VkSampleCountFlagBits max_msaa_samples;
  };

  typedef Context* ContextPtr;

  namespace context {
    Context create(mem::Arena* a, SDL_Window* window, bool debug);
    void    cleanup(Context& ctx, bool debug);

    VkSurfaceFormatKHR surface_format(vulkan::ContextPtr ctx);
  }
}

#endif
