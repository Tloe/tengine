#pragma once

#include "handles.h"

#include "vulkan_include.h"

namespace vulkan {
  namespace render_pass {
    RenderPassHandle create();
    void             cleanup(RenderPassHandle render_pass);

    void
    begin(RenderPassHandle render_pass, CommandBufferHandle command_buffer, VkFramebuffer framebuffer);
    void end(CommandBufferHandle command_buffer);

    VkRenderPass* render_pass(RenderPassHandle render_pass);
  }
}
