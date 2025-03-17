#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace textures {
    TextureHandle create_mipmaped(const char* fpath);
    TextureHandle create_with_staging(U32 w, U32 h);
    void          cleanup(TextureHandle handle);

    void                 set_sampler(TextureHandle handle, TextureSamplerHandle sampler);
    TextureSamplerHandle sampler(TextureHandle handle);
    void                 update_texture(TextureHandle handle, DynamicArray<U32>& data);
  }
}
