#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan_include.h"

namespace vulkan::textures {
  TextureHandle create(U32 w, U32 h, U8* data, U32 byte_size, VkFormat format);
  TextureHandle create_with_staging(U32 w, U32 h);

  TextureHandle load_mipmaped(const char* fpath);

  void cleanup(TextureHandle texture);
  void set_data(TextureHandle texture, U32* data, U32 byte_size);

  void                 set_sampler(TextureHandle texture, vulkan::TextureSamplerHandle sampler);
  TextureSamplerHandle sampler(TextureHandle texture);

}
