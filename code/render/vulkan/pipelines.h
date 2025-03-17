#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace pipelines {
    struct Settings {
      const char* vertex_shader_fpath;
      const char* fragment_shader_fpath;

      VkVertexInputBindingDescription                 binding_description;
      DynamicArray<VkVertexInputAttributeDescription> attribute_descriptions;

      RenderPassHandle render_pass;
      DescriptorSetHandle descriptor_set;
    };

    PipelineHandle create(Settings settings);
    void           cleanup(PipelineHandle handle);
    void           bind(PipelineHandle handle, CommandBufferHandle command_buffer_handle);

    VkPipeline*       pipeline(PipelineHandle handle);
    VkPipelineLayout* layout(PipelineHandle handle);
  }
}
