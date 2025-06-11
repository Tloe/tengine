#include "textures.h"

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/samplers.h"
#include "vulkan/textures.h"
#include "vulkan/ubos.h"

namespace {}

TextureHandle textures::create(U32 w, U32 h, U8* data, U32 byte_size, VkFormat format) {
  return vulkan::textures::create(w, h, data, byte_size, format);
}

TextureHandle textures::create_with_staging(U32 w, U32 h) {
  return vulkan::textures::create_with_staging(w, h);
}

TextureHandle textures::load_mipmaped(const char* fpath) {
  return vulkan::textures::load_mipmaped(fpath);
}

void textures::set_textures(vulkan::UBOHandle ubo, DynamicArray<TextureHandle> textures) {
  vulkan::ubos::set_textures(ubo, textures);
}

void textures::set_data(TextureHandle handle, U32* data, U32 byte_size) {
  return vulkan::textures::set_data(handle, data, byte_size);
}

void textures::cleanup(TextureHandle handle) { return vulkan::textures::cleanup(handle); }

TextureSamplerHandle textures::create_sampler(U32 mip_levels) {
  return vulkan::samplers::create(mip_levels);
}

void textures::set_sampler(TextureHandle handle, vulkan::TextureSamplerHandle sampler) {
  vulkan::textures::set_sampler(handle, sampler);
}

TextureSamplerHandle textures::sampler(TextureHandle handle) {
  return vulkan::textures::sampler(handle);
}

void textures::cleanup(TextureSamplerHandle handle) { return vulkan::samplers::cleanup(handle); }
