#pragma once

#include "types.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan::images {
  struct Size {
    U32 w;
    U32 h;
  };

  ImageHandle create(U16                   w,
                     U16                   h,
                     VkFormat              format,
                     VkImageTiling         tiling,
                     VkImageUsageFlags     usage,
                     VkMemoryPropertyFlags properties,
                     U32                   mip_levels,
                     VkSampleCountFlagBits numSamples);

  ImageHandle add(VkImage vk_image, VkFormat format, U32 mip_levels);

  void cleanup(ImageHandle image);

  void create_view(ImageHandle image, VkImageAspectFlags aspect_flags);
  void cleanup_view(ImageHandle image);

  void transition_layout_safe(ImageHandle image, VkImageLayout old_layout, VkImageLayout new_layout);
  void transition_layout(ImageHandle image, VkImageLayout old_layout, VkImageLayout new_layout);

  void generate_mipmaps(ImageHandle image);

  VkImage*     vk_image(ImageHandle image);
  VkImageView* vk_view(ImageHandle image);
  VkFormat*    vk_format(ImageHandle image);
  U32          mip_levels(ImageHandle image);
  Size         size(ImageHandle image);
}
