#include "samplers.h"

#include "ds_hashmap.h"
#include "vulkan/context.h"
#include "vulkan/device.h"
#include "vulkan/handles.h"
#include "vulkan/vulkan.h"

#include <vulkan/vulkan_core.h>

namespace {
  ArenaHandle          mem_render_resources = arena::by_name("render");
  HashMap16<VkSampler> samplers             = hashmap::init16<VkSampler>(mem_render_resources);
  U16                  next_handle          = 0;
}

vulkan::TextureSamplerHandle vulkan::texture_samplers::create(U32 mip_levels) {
  auto properties = vulkan::device::properties(vulkan::_ctx.physical_device);

  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter               = VK_FILTER_LINEAR;
  sampler_info.minFilter               = VK_FILTER_LINEAR;
  sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable        = VK_TRUE;
  sampler_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
  sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable           = VK_FALSE;
  sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
  // see end of this for these settings
  // https://vulkan-tutorial.com/en/Generating_Mipmaps
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.minLod     = 0.0f; // Optional
  sampler_info.maxLod     = static_cast<F32>(mip_levels);
  sampler_info.mipLodBias = 0.0f; // Optional

  TextureSamplerHandle handle{.value = next_handle++};
  auto                 texture_sampler = hashmap::insert(::samplers, handle.value, VkSampler{});

  ASSERT_SUCCESS(
      "failed to create texture sampler!",
      vkCreateSampler(vulkan::_ctx.logical_device, &sampler_info, nullptr, texture_sampler));

  return handle;
}

void vulkan::texture_samplers::cleanup(TextureSamplerHandle handle) {
  vkDestroySampler(vulkan::_ctx.logical_device, *hashmap::value(::samplers, handle.value), nullptr);
}

VkSampler* vulkan::texture_samplers::sampler(vulkan::TextureSamplerHandle handle) {
  return hashmap::value(::samplers, handle.value);
}
