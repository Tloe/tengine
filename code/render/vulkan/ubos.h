#pragma once

#include "ds_array_dynamic.h"
#include "glm_includes.h"
#include "handles.h"
#include "types.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace ubos {
    struct GlobalUBO {
      glm::mat4 view;
      glm::mat4 proj;
    };

    struct ModelUBO {
      glm::mat4 model;
      U32       texture_index;
    };

    struct PushConstants {
      int texture_index;
    };

    void init(U32 max_textures);
    void cleanup();

    void set_global_ubo(GlobalUBO data);
    void set_model_ubo(ModelUBO data);
    void set_textures(DynamicArray<vulkan::TextureHandle> textures);

    void bind_global_ubo(CommandBufferHandle command_buffer, PipelineHandle pipeline);
    void bind_model_ubo(CommandBufferHandle command_buffer, PipelineHandle pipeline);
    void bind_textures(CommandBufferHandle command_buffer, PipelineHandle pipeline);

    VkDescriptorSetLayout global_ubo_layout();
    VkDescriptorSetLayout model_ubo_layout();
    VkDescriptorSetLayout textures_ubo_layout();
  }
}
