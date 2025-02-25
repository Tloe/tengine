#include "image.h"

#include "arena.h"
#include "ds_hashmap.h"
#include "vulkan/context.h"
#include "vulkan/device.h"
#include "vulkan/vulkan.h"

#include <cstdio>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

namespace {
  struct Images {
    ds::HashMap<U16, VkImage>        images;
    ds::HashMap<U16, VkImageView>    views;
    ds::HashMap<U16, VkDeviceMemory> memory;
    ds::HashMap<U16, VkFormat>       formats;
    ds::HashMap<U16, U32>            mip_levels;
  } images;

  vulkan::ContextPtr ctx;
  U16                next_handle_value = 0;
}

void vulkan::image::init(ContextPtr ctx, mem::ArenaPtr a) {
  images = Images{
      .images     = ds::hashmap::init<U16, VkImage>(a),
      .views      = ds::hashmap::init<U16, VkImageView>(a),
      .memory     = ds::hashmap::init<U16, VkDeviceMemory>(a),
      .formats    = ds::hashmap::init<U16, VkFormat>(a),
      .mip_levels = ds::hashmap::init<U16, U32>(a),
  };

  ::ctx = ctx;
}

vulkan::ImageHandle vulkan::image::create(U32                   width,
                                          U32                   height,
                                          VkFormat              format,
                                          VkImageTiling         tiling,
                                          VkImageUsageFlags     usage,
                                          VkMemoryPropertyFlags properties,
                                          U32                   mip_levels,
                                          VkSampleCountFlagBits num_samples) {
  VkImageCreateInfo image_create_info{};
  image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType     = VK_IMAGE_TYPE_2D;
  image_create_info.extent.width  = width;
  image_create_info.extent.height = height;
  image_create_info.extent.depth  = 1;
  image_create_info.mipLevels     = mip_levels;
  image_create_info.arrayLayers   = 1;
  image_create_info.format        = format;
  image_create_info.tiling        = tiling;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_create_info.usage         = usage;
  image_create_info.samples       = num_samples;
  image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  ImageHandle handle{.value = next_handle_value++};

  auto image = ds::hashmap::insert(images.images, handle.value, VkImage{});

  if (vkCreateImage(ctx->logical_device, &image_create_info, nullptr, image) != VK_SUCCESS) {
    printf("failed to create image!");
    exit(0);
  }

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(ctx->logical_device, *image, &memory_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memory_requirements.size;
  alloc_info.memoryTypeIndex =
      device::memory_type(ctx->physical_device, memory_requirements.memoryTypeBits, properties);

  auto memory = ds::hashmap::insert(images.memory, handle.value, VkDeviceMemory{});

  if (vkAllocateMemory(ctx->logical_device, &alloc_info, nullptr, memory) != VK_SUCCESS) {
    printf("failed to allocate image memory!");
    exit(0);
  }

  vkBindImageMemory(ctx->logical_device, *image, *memory, 0);

  ds::hashmap::insert(images.formats, handle.value, format);
  ds::hashmap::insert(images.mip_levels, handle.value, mip_levels);

  return handle;
}

vulkan::ImageHandle vulkan::image::add(VkImage vk_image, VkFormat format, U32 mip_levels) {
  ImageHandle handle{.value = next_handle_value++};

  ds::hashmap::insert(images.images, handle.value, vk_image);
  ds::hashmap::insert(images.formats, handle.value, format);
  ds::hashmap::insert(images.mip_levels, handle.value, mip_levels);

  return handle;
}

void vulkan::image::cleanup(ImageHandle handle) {
  auto view = image::view(handle);
  if (view != nullptr) {
    vkDestroyImageView(ctx->logical_device, *view, nullptr);
    ds::hashmap::erase(images.views, handle.value);
  }

  auto image = image::image(handle);
  if (image != nullptr) {
    vkDestroyImage(ctx->logical_device, *image, nullptr);
    ds::hashmap::erase(images.images, handle.value);
  }

  auto memory = ds::hashmap::value(images.memory, handle.value);
  if (memory != nullptr) {
    vkFreeMemory(ctx->logical_device, *memory, nullptr);
    ds::hashmap::erase(images.memory, handle.value);
  }
}

void vulkan::image::create_view(ImageHandle handle, VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image                           = *image(handle);
  view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format                          = *format(handle);
  view_create_info.subresourceRange.aspectMask     = aspect_flags;
  view_create_info.subresourceRange.baseMipLevel   = 0;
  view_create_info.subresourceRange.levelCount     = *mip_levels(handle);
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount     = 1;

  auto view = ds::hashmap::insert(images.views, handle.value, VkImageView{});

  if (vkCreateImageView(ctx->logical_device, &view_create_info, nullptr, view) != VK_SUCCESS) {
    printf("failed to create texture image view!");
    exit(0);
  }
}

void vulkan::image::transition_layout(ImageHandle   handle,
                                      VkImageLayout old_layout,
                                      VkImageLayout new_layout) {
  VkCommandBuffer command_buffer = vulkan::begin_single_time_commands(ctx);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = old_layout;
  barrier.newLayout                       = new_layout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = *image(handle);
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = *mip_levels(handle);
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (*format(handle) == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        *format(handle) == VK_FORMAT_D24_UNORM_S8_UINT) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags source_stage      = 0;
  VkPipelineStageFlags destination_stage = 0;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    printf("unsupported layout transition!");
    exit(0);
  }

  vkCmdPipelineBarrier(command_buffer,
                       source_stage,
                       destination_stage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  vulkan::end_single_time_commands(ctx, command_buffer);
}

void vulkan::image::generate_mipmaps(ImageHandle handle, U32 w, U32 h) {
  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(ctx->physical_device, *format(handle), &format_properties);

  if (!(format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    printf("texture image format does not support linear blitting!");
    exit(0);
  }

  VkCommandBuffer command_buffer = vulkan::begin_single_time_commands(ctx);

  auto                 vk_image = *image(handle);
  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = vk_image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  I32 mip_width  = w;
  I32 mip_height = h;

  for (U32 i = 1; i < *mip_levels(handle); i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0]                 = {0, 0, 0};
    blit.srcOffsets[1]                 = {mip_width, mip_height, 1};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = {0, 0, 0};
    blit.dstOffsets[1]                 = {mip_width > 1 ? mip_width / 2 : 1,
                          mip_height > 1 ? mip_height / 2 : 1,
                          1};
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    vkCmdBlitImage(command_buffer,
                   vk_image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   vk_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
    if (mip_width > 1) mip_width /= 2;

    if (mip_height > 1) mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = *mip_levels(handle) - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  vulkan::end_single_time_commands(ctx, command_buffer);
}

VkImage* vulkan::image::image(ImageHandle handle) {
  return ds::hashmap::value(images.images, handle.value);
}

VkImageView* vulkan::image::view(ImageHandle handle) {
  return ds::hashmap::value(images.views, handle.value);
}

VkFormat* vulkan::image::format(ImageHandle handle) {
  return ds::hashmap::value(images.formats, handle.value);
}

U32* vulkan::image::mip_levels(ImageHandle handle) {
  return ds::hashmap::value(images.mip_levels, handle.value);
}
