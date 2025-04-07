#pragma once

#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace pipelines {
    struct Settings {
      const char*                              name;
      const char*                              vertex_shader_fpath;
      const char*                              fragment_shader_fpath;
      const VkVertexInputBindingDescription    binding_description;
      const VkVertexInputAttributeDescription* attribute_descriptions;
      const U32                                attribute_descriptions_format_count = 0;
      VkDescriptorSetLayout*                   ubo_layouts;
      U32                                      ubo_layouts_count;
    };

    PipelineHandle create(Settings settings, RenderPassHandle render_pass);
    void           cleanup(PipelineHandle pipeline);
    void           cleanup();

    void bind(PipelineHandle pipeline, CommandBufferHandle command_buffer);

    PipelineHandle by_name(const char* name);

    VkPipeline*       pipeline(PipelineHandle pipeline);
    VkPipelineLayout* layout(PipelineHandle pipeline);
  }
}
