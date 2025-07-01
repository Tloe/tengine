#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/vulkan_include.h"

namespace textures {
  TextureHandle create(U32 w, U32 h, U8* data, U32 byte_size, VkFormat format);
  TextureHandle create_with_staging(U32 w, U32 h);
  TextureHandle load_mipmaped(const char* fpath);

  void set_textures(vulkan::UBOHandle ubo, DynamicArray<TextureHandle> textures);
  void set_data(TextureHandle handle, U32* data, U32 byte_size);
  void cleanup(TextureHandle handle);

  TextureSamplerHandle create_sampler(U32 mip_levels);
  void                 set_sampler(TextureHandle handle, vulkan::TextureSamplerHandle sampler);
  TextureSamplerHandle sampler(TextureHandle handle);
  void                 cleanup(TextureSamplerHandle handle);
}
