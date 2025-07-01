#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

#include <SDL3/SDL_video.h>
#include "vulkan_include.h"

namespace vulkan {
  extern struct SwapChain {
    VkSwapchainKHR swap_chain;
    VkFormat       image_format;
    VkExtent2D     extent;

    ImageHandle depth;
    ImageHandle multisampling;

    DynamicArray<VkFramebuffer> frame_buffers;
    DynamicArray<ImageHandle>   image_handles;

    SDL_Window*      window;
    RenderPassHandle render_pass;
  } _swap_chain;

  struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR         capabilities = {};
    DynamicArray<VkSurfaceFormatKHR> formats;
    DynamicArray<VkPresentModeKHR>   present_modes;
  };

  namespace swap_chain {
    void create(RenderPassHandle render_pass,SDL_Window* window);
    void cleanup();
    void recreate();

    SwapChainSupport swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
  }
}

