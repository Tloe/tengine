#ifndef IMAGE_H
#define IMAGE_H

#include "arena.h"
#include "handle.h"
#include "types.h"
#include "vulkan/context.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {
  struct ImageTag {};
  typedef Handle<ImageTag, U16, U16_MAX> ImageHandle;
}

namespace vulkan::image {
  void init(ContextPtr ctx, mem::ArenaPtr a);

  ImageHandle create(U32                   w,
                     U32                   h,
                     VkFormat              format,
                     VkImageTiling         tiling,
                     VkImageUsageFlags     usage,
                     VkMemoryPropertyFlags properties,
                     U32                   mip_levels,
                     VkSampleCountFlagBits numSamples);

  ImageHandle add(VkImage vk_image, VkFormat format, U32 mip_levels);

  void cleanup(ImageHandle handle);

  void create_view(ImageHandle handle, VkImageAspectFlags aspectFlags);

  void transition_layout(ImageHandle handle, VkImageLayout oldLayout, VkImageLayout newLayout);

  void generate_mipmaps(ImageHandle handle, U32 w, U32 h);

  VkImage*     image(ImageHandle handle);
  VkImageView* view(ImageHandle handle);
  VkFormat*    format(ImageHandle handle);
  U32*         mip_levels(ImageHandle handle);
}

#endif
