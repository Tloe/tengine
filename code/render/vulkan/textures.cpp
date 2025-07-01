#include "textures.h"

#include "arena.h"
#include "ds_hashmap.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/common.h"
#include "vulkan/context.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan_include.h"

#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace {
  auto mem_render = arena::by_name("render");

  auto samplers                = hashmap::init16<vulkan::TextureSamplerHandle>(mem_render);
  auto staging_buffers         = hashmap::init16<vulkan::BufferHandle>(mem_render);
  auto staging_command_buffers = hashmap::init16<vulkan::CommandBufferHandle>(mem_render);
}

vulkan::TextureHandle
vulkan::textures::create(U32 w, U32 h, U8* data, U32 byte_size, VkFormat format) {
  auto staging = vulkan::buffers::create(byte_size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  vulkan::buffers::copy(staging, data, byte_size);

  auto texture_image =
      vulkan::images::create(w,
                             h,
                             format,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             1,
                             VK_SAMPLE_COUNT_1_BIT);

  vulkan::images::transition_layout(texture_image,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vulkan::buffers::copy(texture_image, staging, w, h);

  vulkan::images::transition_layout(texture_image,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vulkan::buffers::cleanup(staging);

  vulkan::images::create_view(texture_image, VK_IMAGE_ASPECT_COLOR_BIT);

  return texture_image;
}

vulkan::TextureHandle vulkan::textures::load_mipmaped(const char* fpath) {
  int      w, h, texture_channels;
  stbi_uc* pixels          = stbi_load(fpath, &w, &h, &texture_channels, STBI_rgb_alpha);
  U32      image_byte_size = w * h * 4;

  U32 mip_levels = std::floor(std::log2(std::max(w, h))) + 1;

  if (!pixels) {
    printf("failed to load texture image!\n");
    exit(0);
  }

  auto staging = vulkan::buffers::create(image_byte_size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  vulkan::buffers::copy(staging, pixels, image_byte_size);

  stbi_image_free(pixels);

  auto texture_image =
      vulkan::images::create(w,
                             h,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             mip_levels,
                             VK_SAMPLE_COUNT_1_BIT);

  vulkan::images::transition_layout(texture_image,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vulkan::buffers::copy(texture_image, staging, w, h);

  vulkan::buffers::cleanup(staging);

  vulkan::images::create_view(texture_image, VK_IMAGE_ASPECT_COLOR_BIT);

  vulkan::images::generate_mipmaps(texture_image);

  return texture_image;
}

vulkan::TextureHandle vulkan::textures::create_with_staging(U32 w, U32 h) {
  auto staging = vulkan::buffers::create(sizeof(U32) * w * h,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  auto texture_image =
      vulkan::images::create(w,
                             h,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             1,
                             VK_SAMPLE_COUNT_1_BIT);

  vulkan::images::create_view(texture_image, VK_IMAGE_ASPECT_COLOR_BIT);

  hashmap::insert(staging_buffers, texture_image.value, staging);

  auto command_buffer = command_buffers::create();
  hashmap::insert(staging_command_buffers, texture_image.value, command_buffer);

  return TextureHandle{.value = texture_image.value};
}

void vulkan::textures::cleanup(TextureHandle handle) {
  vulkan::images::cleanup(vulkan::ImageHandle{.value = handle.value});

  if (hashmap::contains(staging_buffers, handle.value)) {
    vulkan::buffers::cleanup(*hashmap::value(staging_buffers, handle.value));
    hashmap::erase(staging_buffers, handle.value);
  }

  hashmap::erase(staging_command_buffers, handle.value);
}

void vulkan::textures::set_sampler(TextureHandle handle, TextureSamplerHandle sampler) {
  hashmap::insert(samplers, handle.value, sampler);
}

vulkan::TextureSamplerHandle vulkan::textures::sampler(TextureHandle handle) {
  return *hashmap::value(samplers, handle.value);
}

void vulkan::textures::set_data(TextureHandle texture, U32* data, U32 byte_size) {
  assert(hashmap::contains(staging_buffers, texture.value) &&
         hashmap::contains(staging_command_buffers, texture.value) &&
         "texture not created with staging buffer");

  auto staging_buffer         = *hashmap::value(staging_buffers, texture.value);
  auto staging_command_buffer = *hashmap::value(staging_command_buffers, texture.value);
  auto vk_staging_memory      = *buffers::vk_memory(staging_buffer);
  auto vk_staging_buffer      = *buffers::vk_buffer(staging_buffer);
  auto size                   = images::size(texture);

  void* mapped_data;
  ASSERT_SUCCESS(
      "failed to map memory",
      vkMapMemory(vulkan::_ctx.logical_device, vk_staging_memory, 0, byte_size, 0, &mapped_data));

  memcpy(mapped_data, data, byte_size);
  vkUnmapMemory(vulkan::_ctx.logical_device, vk_staging_memory);

  command_buffers::reset(staging_command_buffer);
  auto vk_command_buffer = command_buffers::begin(staging_command_buffer);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED; // or previous layout
  barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = *images::vk_image(texture);
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.srcAccessMask                   = 0;
  barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(*vk_command_buffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  // 3. Copy the staging buffer to the image
  VkBufferImageCopy region{};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0; // Tightly packed
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {size.w, size.h, 1};

  vkCmdCopyBufferToImage(*vk_command_buffer,
                         vk_staging_buffer,
                         *images::vk_image(texture),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(*vk_command_buffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  command_buffers::submit(staging_command_buffer);
}
