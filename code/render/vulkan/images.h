#pragma once

#include "handles.h"
#include "types.h"

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

  void cleanup(ImageHandle handle);

  void create_view(ImageHandle handle, VkImageAspectFlags aspect_flags);
  void cleanup_view(ImageHandle handle);

  void transition_layout(ImageHandle handle, VkImageLayout old_layout, VkImageLayout new_layout);

  void generate_mipmaps(ImageHandle handle);

  VkImage*     image(ImageHandle handle);
  VkImageView* view(ImageHandle handle);
  VkFormat*    format(ImageHandle handle);
  U32*         mip_levels(ImageHandle handle);
  Size         size(ImageHandle handle);
}
