#ifndef RENDER_H
#define RENDER_H

#include "vulkan/swap_chain.h"
#include "vulkan/context.h"
#include <vulkan/vulkan_core.h>

namespace mem {
  struct Arena;
  typedef Arena* ArenaPtr;
}

namespace render {
  struct Renderer {
    vulkan::Context ctx;
    vulkan::SwapChain swap_chain;
    VkRenderPass render_pass;
    SDL_Window *window;
  };

  Renderer init(mem::ArenaPtr a);
  void update(Renderer& renderer, mem::ArenaPtr a);
  void cleanup(Renderer& renderer);
}

#endif
