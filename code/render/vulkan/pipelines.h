#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace pipelines {
    struct Settings {
      const char*                              vertex_shader_fpath;
      const char*                              fragment_shader_fpath;
      const VkVertexInputBindingDescription    binding_description;
      const VkVertexInputAttributeDescription* attribute_descriptions;
      const U32                                attribute_descriptions_format_count = 0;
      DynamicArray<vulkan::UBOHandle>          ubos;
      bool                                     disable_depth_testing = false;
    };

    PipelineHandle create(RenderPassHandle render_pass, Settings settings);
    void           cleanup(PipelineHandle pipeline);
    void           cleanup();

    void bind(PipelineHandle pipeline, CommandBufferHandle command_buffer);

    VkPipeline*       pipeline(PipelineHandle pipeline);
    VkPipelineLayout* layout(PipelineHandle pipeline);
  }
}
