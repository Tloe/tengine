#include "images.h"

#include "arena.h"
#include "command_buffers.h"
#include "common.h"
#include "context.h"
#include "device.h"
#include "ds_hashmap.h"
#include "handle.h"
#include "types.h"
#include "vulkan/handles.h"
#include "vulkan_include.h"

#include <cstdio>
#include <cstdlib>

namespace {
  struct ImageData {
    VkImage              vk_image   = VK_NULL_HANDLE;
    VkImageView          vk_view    = VK_NULL_HANDLE;
    VkDeviceMemory       vk_memory  = VK_NULL_HANDLE;
    VkFormat             vk_format  = VK_FORMAT_UNDEFINED;
    U32                  mip_levels = 1;
    vulkan::images::Size size       = {};
  };

  ArenaHandle mem_render = arena::by_name("render");

  auto _image_datas = hashmap::init16<ImageData>(mem_render);

  handles::Allocator<vulkan::ImageHandle, U16, 20> _handles;
}

vulkan::ImageHandle vulkan::images::create(U16                   w,
                                           U16                   h,
                                           VkFormat              format,
                                           VkImageTiling         tiling,
                                           VkImageUsageFlags     usage,
                                           VkMemoryPropertyFlags properties,
                                           U32                   mip_levels,
                                           VkSampleCountFlagBits num_samples) {
  VkImageCreateInfo create_info{};
  create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create_info.imageType     = VK_IMAGE_TYPE_2D;
  create_info.extent.width  = w;
  create_info.extent.height = h;
  create_info.extent.depth  = 1;
  create_info.mipLevels     = mip_levels;
  create_info.arrayLayers   = 1;
  create_info.format        = format;
  create_info.tiling        = tiling;
  create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  create_info.usage         = usage;
  create_info.samples       = num_samples;
  create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  auto image = handles::next(_handles);

  auto image_data = hashmap::insert(_image_datas,
                                    image.value,
                                    ImageData{
                                        .vk_format  = format,
                                        .mip_levels = mip_levels,
                                        .size       = {w, h},
                                    });

  ASSERT_SUCCESS(
      "failed to create image!",
      vkCreateImage(vulkan::_ctx.logical_device, &create_info, nullptr, &image_data->vk_image));

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(vulkan::_ctx.logical_device,
                               image_data->vk_image,
                               &memory_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize  = memory_requirements.size;
  alloc_info.memoryTypeIndex = device::memory_type(vulkan::_ctx.physical_device,
                                                   memory_requirements.memoryTypeBits,
                                                   properties);

  ASSERT_SUCCESS(
      "failed to allocate image memory!",
      vkAllocateMemory(vulkan::_ctx.logical_device, &alloc_info, nullptr, &image_data->vk_memory));

  vkBindImageMemory(vulkan::_ctx.logical_device, image_data->vk_image, image_data->vk_memory, 0);

  return image;
}

vulkan::ImageHandle vulkan::images::add(VkImage vk_image, VkFormat vk_format, U32 mip_levels) {
  auto image = handles::next(_handles);

  hashmap::insert(_image_datas,
                  image.value,
                  ImageData{
                      .vk_image   = vk_image,
                      .vk_format  = vk_format,
                      .mip_levels = mip_levels,
                  });

  return image;
}

void vulkan::images::remove(vulkan::ImageHandle image) {
  handles::free(_handles, image);
  hashmap::erase(_image_datas, image.value);
}

void vulkan::images::cleanup(ImageHandle image) {
  auto image_data = hashmap::value(_image_datas, image.value);

  if (image_data->vk_view != VK_NULL_HANDLE) {
    vkDestroyImageView(vulkan::_ctx.logical_device, image_data->vk_view, nullptr);
  }

  if (image_data->vk_image != VK_NULL_HANDLE) {
    vkDestroyImage(vulkan::_ctx.logical_device, image_data->vk_image, nullptr);
  }

  if (image_data->vk_memory != VK_NULL_HANDLE) {
    vkFreeMemory(vulkan::_ctx.logical_device, image_data->vk_memory, nullptr);
  }

  handles::free(_handles, image);

  hashmap::erase(_image_datas, image.value);
}

void vulkan::images::create_view(ImageHandle handle, VkImageAspectFlags aspect_flags) {
  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image                           = *vk_image(handle);
  view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format                          = *vk_format(handle);
  view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
  view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
  view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
  view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
  view_create_info.subresourceRange.aspectMask     = aspect_flags;
  view_create_info.subresourceRange.baseMipLevel   = 0;
  view_create_info.subresourceRange.levelCount     = mip_levels(handle);
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount     = 1;

  auto image_data = hashmap::value(_image_datas, handle.value);

  ASSERT_SUCCESS("failed to create texture image view!",
                 vkCreateImageView(vulkan::_ctx.logical_device,
                                   &view_create_info,
                                   nullptr,
                                   &image_data->vk_view));
}

void vulkan::images::cleanup_view(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  if (image_data->vk_view != VK_NULL_HANDLE) {
    vkDestroyImageView(vulkan::_ctx.logical_device, image_data->vk_view, nullptr);
    image_data->vk_view = VK_NULL_HANDLE;
  }
}

void vulkan::images::transition_layout(ImageHandle   handle,
                                       VkImageLayout old_layout,
                                       VkImageLayout new_layout) {
  auto command_buffer = command_buffers::create();
  command_buffers::begin(command_buffer);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = old_layout;
  barrier.newLayout                       = new_layout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = *vk_image(handle);
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = mip_levels(handle);
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (*vk_format(handle) == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        *vk_format(handle) == VK_FORMAT_D24_UNORM_S8_UINT) {
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

  vkCmdPipelineBarrier(*vulkan::command_buffers::buffer(command_buffer),
                       source_stage,
                       destination_stage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  command_buffers::submit(command_buffer);
}

void vulkan::images::generate_mipmaps(ImageHandle handle) {
  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(vulkan::_ctx.physical_device,
                                      *vk_format(handle),
                                      &format_properties);

  if (!(format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    printf("texture image format does not support linear blitting!");
    exit(0);
  }

  auto command_buffer    = command_buffers::create();
  auto vk_command_buffer = command_buffers::begin(command_buffer);

  auto                 vk_image = *images::vk_image(handle);
  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = vk_image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  auto image_data = hashmap::value(_image_datas, handle.value);

  I32 mip_width  = image_data->size.w;
  I32 mip_height = image_data->size.h;

  for (U32 i = 1; i < mip_levels(handle); i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(*vk_command_buffer,
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

    vkCmdBlitImage(*vk_command_buffer,
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

    if (mip_width > 1) mip_width /= 2;

    if (mip_height > 1) mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels(handle) - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

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

  command_buffers::submit(command_buffer);
}

VkImage* vulkan::images::vk_image(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  return &image_data->vk_image;
}

VkImageView* vulkan::images::vk_view(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  return &image_data->vk_view;
}

VkFormat* vulkan::images::vk_format(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  return &image_data->vk_format;
}

U32 vulkan::images::mip_levels(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  return image_data->mip_levels;
}

vulkan::images::Size vulkan::images::size(ImageHandle handle) {
  auto image_data = hashmap::value(_image_datas, handle.value);

  return image_data->size;
}
