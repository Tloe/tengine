#pragma once

#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace render_pass {
    RenderPassHandle create();
    void             cleanup(RenderPassHandle handle);

    void
    begin(RenderPassHandle handle, CommandBufferHandle command_buffer, VkFramebuffer framebuffer);
    void end(CommandBufferHandle command_buffer);

    VkRenderPass* render_pass(RenderPassHandle handle);
  }
}
