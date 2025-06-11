#pragma once

#include "ds_array_dynamic.h"
#include "glm_includes.h"
#include "handles.h"
#include "types.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  struct GlobalUBO {
    glm::mat4 view;
    glm::mat4 proj;
  };

  namespace ubos {
    struct Settings {
      U32 ubo_count         = 0;
      U32 max_textures      = 0;
      U32 texture_set_count = 0;
    };

    void init(Settings settings);
    void cleanup();

    UBOHandle create_ubo(U8 set_index, U32 byte_size, U32 descriptor_count = 1);
    UBOHandle create_texture_set(U8 set_index, U32 texture_count);

    void cleanup(UBOHandle ubo);

    void set_ubo(UBOHandle ubo, void* data);
    void set_textures(UBOHandle ubo, DynamicArray<vulkan::TextureHandle> textures);
    U32  texture_index(vulkan::TextureHandle texture);

    void bind(UBOHandle ubo, CommandBufferHandle command_buffer, PipelineHandle pipeline);

    VkDescriptorSetLayout descriptor_set_layout(UBOHandle ubo);
  }
}
