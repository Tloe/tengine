#ifndef VULKAN_H
#define VULKAN_H

#include "ds_array_dynamic.h"
#include "vulkan/context.h"
#include <cstdlib>
#include <vulkan/vulkan_core.h>

namespace ds {
  struct String;
}

namespace vulkan {
  constexpr const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  constexpr const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  ds::DynamicArray<const char *> required_extension(mem::Arena *perm, bool debug);
  ds::DynamicArray<ds::String> available_extensions(mem::Arena *perm, VkPhysicalDevice device);


  bool queue_family_indices(VkPhysicalDevice device,
                            VkSurfaceKHR surface,
                            U32 *graphicsFamily,
                            U32 *presentFamily);

  VkCommandBuffer begin_single_time_commands(ContextPtr ctx);

  void
  end_single_time_commands(ContextPtr ctx, VkCommandBuffer command_buffer);
}

#endif
