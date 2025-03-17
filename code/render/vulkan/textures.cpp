#include "textures.h"

#include "arena.h"
#include "ds_hashmap.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/context.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan/vulkan.h"

#include <cmath>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace {
  auto mem_render_resources = arena::by_name("render_resources");

  auto samplers        = hashmap::init16<vulkan::TextureSamplerHandle>(mem_render_resources);
  auto staging_buffers = hashmap::init16<vulkan::BufferHandle>(mem_render_resources);
  auto staging_command_buffers = hashmap::init16<vulkan::CommandBufferHandle>(mem_render_resources);
}

vulkan::TextureHandle vulkan::textures::create_mipmaped(const char* fpath) {
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

  auto texture_image_handle =
      vulkan::images::create(w,
                             h,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             mip_levels,
                             VK_SAMPLE_COUNT_1_BIT);

  vulkan::images::transition_layout(texture_image_handle,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vulkan::buffers::copy(texture_image_handle, staging, w, h);

  vulkan::buffers::cleanup(staging);

  vulkan::images::create_view(texture_image_handle, VK_IMAGE_ASPECT_COLOR_BIT);

  vulkan::images::generate_mipmaps(texture_image_handle);

  return TextureHandle{.value = texture_image_handle.value};
}

vulkan::TextureHandle vulkan::textures::create_with_staging(U32 w, U32 h) {
  auto staging = vulkan::buffers::create(sizeof(U32) * w * h,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  auto texture_image_handle =
      vulkan::images::create(w,
                             h,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             1,
                             VK_SAMPLE_COUNT_1_BIT);

  vulkan::images::create_view(texture_image_handle, VK_IMAGE_ASPECT_COLOR_BIT);

  hashmap::insert(staging_buffers, texture_image_handle.value, staging);

  auto command_buffer = command_buffers::create();
  hashmap::insert(staging_command_buffers, texture_image_handle.value, command_buffer);
  printf("create staging command_buffer %d\n", command_buffer.value);

  return TextureHandle{.value = texture_image_handle.value};
}

void vulkan::textures::cleanup(TextureHandle handle) {
  vulkan::images::cleanup(vulkan::ImageHandle{.value = handle.value});

  if (hashmap::contains(staging_buffers, handle.value)) {
    vulkan::buffers::cleanup(*hashmap::value(staging_buffers, handle.value));
    hashmap::erase(staging_buffers, handle.value);
  }

  if (hashmap::contains(staging_command_buffers, handle.value)) {
    vulkan::command_buffers::cleanup(*hashmap::value(staging_command_buffers, handle.value));
    hashmap::erase(staging_command_buffers, handle.value);
  }
}

void vulkan::textures::set_sampler(TextureHandle handle, TextureSamplerHandle sampler) {
  hashmap::insert(samplers, handle.value, sampler);
}

vulkan::TextureSamplerHandle vulkan::textures::sampler(TextureHandle handle) {
  return *hashmap::value(samplers, handle.value);
}

void vulkan::textures::update_texture(TextureHandle handle, DynamicArray<U32>& updated_data) {
  assert(hashmap::contains(staging_buffers, handle.value) &&
         hashmap::contains(staging_command_buffers, handle.value) &&
         "texture not created with staging buffer");

  auto staging_buffer         = *hashmap::value(staging_buffers, handle.value);
  auto staging_command_buffer = *hashmap::value(staging_command_buffers, handle.value);
  auto vk_staging_memory      = *buffers::memory(staging_buffer);
  auto vk_staging_buffer      = *buffers::buffer(staging_buffer);
  auto size                   = images::size(handle);

  void* data;
  ASSERT_SUCCESS("failed to map memory",
                 vkMapMemory(vulkan::_ctx.logical_device,
                             vk_staging_memory,
                             0,
                             updated_data._size * sizeof(U32),
                             0,
                             &data));

  memcpy(data, updated_data._data, updated_data._size * sizeof(U32));
  vkUnmapMemory(vulkan::_ctx.logical_device, vk_staging_memory);

  command_buffers::reset(staging_command_buffer);
  auto vk_command_buffer = command_buffers::begin(staging_command_buffer);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED; // or previous layout
  barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = *images::image(handle);
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
                         *images::image(handle),
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
