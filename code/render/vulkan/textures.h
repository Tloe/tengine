#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan::textures {
TextureHandle create_mipmaped(const char *fpath);
TextureHandle create_with_staging(U32 w, U32 h);
void cleanup(TextureHandle handle);
void set(TextureHandle handle, DynamicArray<U32> &data);

void set_sampler(TextureHandle handle, vulkan::TextureSamplerHandle sampler);
TextureSamplerHandle sampler(TextureHandle handle);

} // namespace vulkan::textures
