#include "textures.h"
#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/textures.h"
#include "vulkan/samplers.h"
#include "vulkan/ubos.h"

namespace {
  
}

TextureHandle textures::create_mipmaped(const char *fpath) {
  return vulkan::textures::create_mipmaped(fpath);
}

TextureHandle textures::create_with_staging(U32 w, U32 h) {
  return vulkan::textures::create_with_staging(w, h);
}

void textures::set_textures(DynamicArray<TextureHandle> textures) {
  vulkan::ubos::set_textures(textures);
}

void textures::set_data(TextureHandle handle, DynamicArray<U32> &data) {
  return vulkan::textures::set(handle, data);
}

void textures::cleanup(TextureHandle handle) {
  return vulkan::textures::cleanup(handle);
}

TextureSamplerHandle textures::create_sampler(U32 mip_levels) {
  return vulkan::samplers::create(mip_levels);
}

void textures::set_sampler(TextureHandle handle,
                           vulkan::TextureSamplerHandle sampler) {
  vulkan::textures::set_sampler(handle, sampler);
}

TextureSamplerHandle textures::sampler(TextureHandle handle) {
  return vulkan::textures::sampler(handle);
}

void textures::cleanup(TextureSamplerHandle handle) {
  return vulkan::samplers::cleanup(handle);
}

