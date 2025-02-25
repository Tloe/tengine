#include "swap_chain.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "vulkan.h"
#include "vulkan/context.h"
#include "vulkan/device.h"
#include "vulkan/image.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glm/common.hpp>
#include <vulkan/vulkan_core.h>

namespace {
  VkPresentModeKHR choose_swap_present_mode(
      const ds::DynamicArray<VkPresentModeKHR>& available_present_modes) {
    for (U32 i = 0; i < available_present_modes._size; i++) {
      if (available_present_modes._data[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        return available_present_modes._data[i];
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D choose_swap_extent(SDL_Window*                     window,
                                const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      SDL_GetWindowSize(window, &width, &height);

      VkExtent2D actual_extent = {static_cast<uint32_t>(width),
                                  static_cast<uint32_t>(height)};

      actual_extent.width  = glm::clamp(actual_extent.width,
                                       capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
      actual_extent.height = glm::clamp(actual_extent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);

      return actual_extent;
    }
  }

  vulkan::ImageHandle
  create_depth_image(vulkan::ContextPtr ctx, U32 width, U32 height) {
    VkFormat depth_format =
        vulkan::device::find_depth_format(ctx->physical_device);

    auto image_handle =
        vulkan::image::create(width,
                              height,
                              depth_format,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              1,
                              ctx->max_msaa_samples);

    vulkan::image::create_view(image_handle, VK_IMAGE_ASPECT_DEPTH_BIT);

    vulkan::image::transition_layout(
        image_handle,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return image_handle;
  }

  vulkan::ImageHandle create_multisampling(vulkan::ContextPtr ctx,
                                           VkFormat           color_format,
                                           U32                width,
                                           U32                height) {
    auto image_handle =
        vulkan::image::create(width,
                              height,
                              color_format,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              1,
                              ctx->max_msaa_samples);

    vulkan::image::create_view(image_handle, VK_IMAGE_ASPECT_COLOR_BIT);

    return image_handle;
  }
}

vulkan::SwapChain vulkan::swap_chain::create(ContextPtr    ctx,
                                             mem::ArenaPtr a,
                                             SDL_Window*   window) {
  auto swap_chain_support =
      vulkan::swap_chain::swap_chain_support(a,
                                             ctx->physical_device,
                                             ctx->surface);

  VkSurfaceFormatKHR surface_format = vulkan::context::surface_format(ctx);
  VkPresentModeKHR   presentMode =
      ::choose_swap_present_mode(swap_chain_support.present_modes);

  VkExtent2D extent =
      choose_swap_extent(window, swap_chain_support.capabilities);

  U32 image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 &&
      image_count > swap_chain_support.capabilities.maxImageCount) {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface          = ctx->surface;
  create_info.minImageCount    = image_count;
  create_info.imageFormat      = surface_format.format;
  create_info.imageColorSpace  = surface_format.colorSpace;
  create_info.imageExtent      = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  U32 queue_family_indices[2];
  vulkan::queue_family_indices(ctx->physical_device,
                               ctx->surface,
                               &queue_family_indices[0],
                               &queue_family_indices[1]);

  if (queue_family_indices[0] != queue_family_indices[1]) {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform   = swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode    = presentMode;
  create_info.clipped        = VK_TRUE;
  create_info.oldSwapchain   = VK_NULL_HANDLE;

  auto depth         = create_depth_image(ctx, extent.width, extent.height);
  auto multisampling = create_multisampling(ctx,
                                            surface_format.format,
                                            extent.width,
                                            extent.height);

  vulkan::SwapChain swap_chain = {
      .depth         = depth,
      .multisampling = multisampling,
      .frame_buffers =
          ds::array::init<VkFramebuffer>(a, image_count, image_count),
      .image_handles =
          ds::array::init<ImageHandle>(a, image_count, image_count),
  };

  if (vkCreateSwapchainKHR(ctx->logical_device,
                           &create_info,
                           nullptr,
                           &swap_chain.swap_chain) != VK_SUCCESS) {
    printf("failed to create swap chain!");
    exit(0);
  }

  vkGetSwapchainImagesKHR(ctx->logical_device,
                          swap_chain.swap_chain,
                          &image_count,
                          nullptr);
  VkImage images[image_count];
  vkGetSwapchainImagesKHR(ctx->logical_device,
                          swap_chain.swap_chain,
                          &image_count,
                          images);

  for (U32 i = 0; i < image_count; i++) {
    swap_chain.image_handles._data[i] =
        vulkan::image::add(images[i], surface_format.format, 1);
  }

  swap_chain.image_format = surface_format.format;
  swap_chain.extent       = extent;

  for (U32 i = 0; i < image_count; i++) {
    vulkan::image::create_view(swap_chain.image_handles._data[i],
                               VK_IMAGE_ASPECT_COLOR_BIT);
  }

  return swap_chain;
}

void
vulkan::swap_chain::cleanup(vulkan::ContextPtr ctx, SwapChainPtr swap_chain) {
  vulkan::image::cleanup(swap_chain->multisampling);
  vulkan::image::cleanup(swap_chain->depth);

  for (U32 i = 0; i < swap_chain->frame_buffers._size; i++) {
    vkDestroyFramebuffer(ctx->logical_device,
                         swap_chain->frame_buffers._data[i],
                         nullptr);
  }

  for (U32 i = 0; i < swap_chain->image_handles._size; i++) {
    vkDestroyImageView(ctx->logical_device,
                       *image::view(swap_chain->image_handles._data[i]),
                       nullptr);
  }

  vkDestroySwapchainKHR(ctx->logical_device, swap_chain->swap_chain, nullptr);
}

vulkan::SwapChainSupport
vulkan::swap_chain::swap_chain_support(mem::Arena*      perm,
                                       VkPhysicalDevice device,
                                       VkSurfaceKHR     surface) {
  SwapChainSupport details{
      .capabilities  = VkSurfaceCapabilitiesKHR{},
      .formats       = ds::array::init<VkSurfaceFormatKHR>(perm),
      .present_modes = ds::array::init<VkPresentModeKHR>(perm),
  };

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,
                                            surface,
                                            &details.capabilities);

  U32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  if (format_count != 0) {
    ds::array::resize(details.formats, format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                                         surface,
                                         &format_count,
                                         details.formats._data);
  }

  U32 present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                            surface,
                                            &present_mode_count,
                                            nullptr);
  if (present_mode_count != 0) {
    ds::array::resize(details.present_modes, present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                              surface,
                                              &present_mode_count,
                                              details.present_modes._data);
  }

  return details;
}

void vulkan::swap_chain::create_framebuffers(vulkan::ContextPtr   ctx,
                                             VkRenderPass         render_pass,
                                             vulkan::SwapChainPtr swap_chain) {
  for (U32 i = 0; i < swap_chain->image_handles._size; i++) {
    VkImageView attachments[] = {
        *image::view(swap_chain->multisampling),
        *image::view(swap_chain->depth),
        *image::view(swap_chain->image_handles._data[i])};

    VkFramebufferCreateInfo create_info{};
    create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass      = render_pass;
    create_info.attachmentCount = ds::array::size(attachments);
    create_info.pAttachments    = attachments;
    create_info.width           = swap_chain->extent.width;
    create_info.height          = swap_chain->extent.height;
    create_info.layers          = 1;

    swap_chain->frame_buffers._data[i] = VK_NULL_HANDLE;

    if (vkCreateFramebuffer(ctx->logical_device,
                            &create_info,
                            nullptr,
                            &swap_chain->frame_buffers._data[i]) !=
        VK_SUCCESS) {
      printf("failed to create framebuffer!");
      exit(0);
    }
  }
}
