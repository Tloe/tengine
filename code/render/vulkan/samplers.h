#pragma once

#include "vulkan/handles.h"
#include <vulkan/vulkan_core.h>

namespace vulkan {
  namespace samplers {
    TextureSamplerHandle create(U32 mip_levels);
    void                 cleanup(TextureSamplerHandle handle);

    VkSampler* sampler(TextureSamplerHandle handle);
  }
  typedef vulkan::ImageHandle TextureHandle;
}
