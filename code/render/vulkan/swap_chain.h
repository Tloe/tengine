#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include "ds_array_dynamic.h"
#include "vulkan/image.h"

#include <vulkan/vulkan_core.h>

namespace mem { struct Arena; }
namespace mem { typedef Arena* ArenaPtr; }
namespace vulkan { struct Context; }
namespace vulkan { typedef Context* ContextPtr; }

namespace vulkan {
  struct SwapChain {
    VkSwapchainKHR swap_chain;
    VkFormat       image_format;
    VkExtent2D     extent;

    ImageHandle depth;
    ImageHandle multisampling;

    ds::DynamicArray<VkFramebuffer> frame_buffers;
    ds::DynamicArray<ImageHandle>   image_handles;
  };
  typedef SwapChain* SwapChainPtr;

  struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR             capabilities = {};
    ds::DynamicArray<VkSurfaceFormatKHR> formats;
    ds::DynamicArray<VkPresentModeKHR>   present_modes;
  };

  namespace swap_chain {
    SwapChain create(ContextPtr ctx, mem::ArenaPtr a, SDL_Window* window);
    void      cleanup(vulkan::ContextPtr ctx, SwapChainPtr swap_chain);

    void create_framebuffers(vulkan::ContextPtr   ctx,
                             VkRenderPass         render_pass,
                             vulkan::SwapChainPtr swap_chain);

    SwapChainSupport
    swap_chain_support(mem::Arena* perm, VkPhysicalDevice device, VkSurfaceKHR surface);
  }
}

#endif
