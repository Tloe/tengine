#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

namespace textures {
  TextureHandle create_mipmaped(const char* fpath);
  TextureHandle create_with_staging(U32 w, U32 h);
  void          set_textures(DynamicArray<TextureHandle> textures);
  void          set_data(TextureHandle handle, DynamicArray<U32>& data);
  void          cleanup(TextureHandle handle);

  TextureSamplerHandle create_sampler(U32 mip_levels);
  void                 set_sampler(TextureHandle handle, vulkan::TextureSamplerHandle sampler);
  TextureSamplerHandle sampler(TextureHandle handle);
  void                 cleanup(TextureSamplerHandle handle);
}
