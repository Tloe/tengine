#include "swap_chain.h"

#include "arena.h"
#include "context.h"
#include "device.h"
#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "handles.h"
#include "images.h"
#include "render_pass.h"
#include "common.h"

#include <SDL3/SDL_events.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glm/common.hpp>
#include "vulkan_include.h"

namespace vulkan { SwapChain _swap_chain = {}; }

namespace {
  auto _mem_render = arena::by_name("render");

  VkPresentModeKHR
  choose_swap_present_mode(const DynamicArray<VkPresentModeKHR>& available_present_modes) {
    for (U32 i = 0; i < available_present_modes._size; i++) {
      if (available_present_modes._data[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        return available_present_modes._data[i];
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D choose_swap_extent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      SDL_GetWindowSize(window, &width, &height);

      VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      actual_extent.width  = glm::clamp(actual_extent.width,
                                       capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
      actual_extent.height = glm::clamp(actual_extent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);

      return actual_extent;
    }
  }

  vulkan::ImageHandle create_depth_image(U32 width, U32 height) {
    VkFormat depth_format = vulkan::device::find_depth_format(vulkan::_ctx.physical_device);

    auto image_handle = vulkan::images::create(width,
                                               height,
                                               depth_format,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               1,
                                               vulkan::_ctx.max_msaa_samples);

    vulkan::images::create_view(image_handle, VK_IMAGE_ASPECT_DEPTH_BIT);

    vulkan::images::transition_layout(image_handle,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return image_handle;
  }

  vulkan::ImageHandle create_multisampling(VkFormat color_format, U32 width, U32 height) {
    auto image_handle = vulkan::images::create(width,
                                               height,
                                               color_format,
                                               VK_IMAGE_TILING_OPTIMAL,
                                               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               1,
                                               vulkan::_ctx.max_msaa_samples);

    vulkan::images::create_view(image_handle, VK_IMAGE_ASPECT_COLOR_BIT);

    return image_handle;
  }

  void create_framebuffers(vulkan::RenderPassHandle render_pass) {
    for (U32 i = 0; i < vulkan::_swap_chain.image_handles._size; i++) {
      VkImageView attachments[] = {
          *vulkan::images::vk_view(vulkan::_swap_chain.multisampling),
          *vulkan::images::vk_view(vulkan::_swap_chain.depth),
          *vulkan::images::vk_view(vulkan::_swap_chain.image_handles._data[i])};

      VkFramebufferCreateInfo create_info{};
      create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      create_info.renderPass      = *vulkan::render_pass::render_pass(render_pass);
      create_info.attachmentCount = array::size(attachments);
      create_info.pAttachments    = attachments;
      create_info.width           = vulkan::_swap_chain.extent.width;
      create_info.height          = vulkan::_swap_chain.extent.height;
      create_info.layers          = 1;

      vulkan::_swap_chain.frame_buffers._data[i] = VK_NULL_HANDLE;

      ASSERT_SUCCESS("failed to create framebuffer!",
                     vkCreateFramebuffer(vulkan::_ctx.logical_device,
                                         &create_info,
                                         nullptr,
                                         &vulkan::_swap_chain.frame_buffers._data[i]));
    }
  }
}

void vulkan::swap_chain::create(vulkan::RenderPassHandle render_pass, SDL_Window* window) {
  auto swap_chain_support =
      vulkan::swap_chain::swap_chain_support(vulkan::_ctx.physical_device, vulkan::_ctx.surface);

  VkSurfaceFormatKHR surface_format = vulkan::context::surface_format();
  VkPresentModeKHR   presentMode    = ::choose_swap_present_mode(swap_chain_support.present_modes);

  VkExtent2D extent      = choose_swap_extent(window, swap_chain_support.capabilities);
  U32        image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 &&
      image_count > swap_chain_support.capabilities.maxImageCount) {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface          = vulkan::_ctx.surface;
  create_info.minImageCount    = image_count;
  create_info.imageFormat      = surface_format.format;
  create_info.imageColorSpace  = surface_format.colorSpace;
  create_info.imageExtent      = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  U32 queue_family_indices[2];
  vulkan::queue_family_indices(vulkan::_ctx.physical_device,
                               vulkan::_ctx.surface,
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

  auto depth         = create_depth_image(extent.width, extent.height);
  auto multisampling = create_multisampling(surface_format.format, extent.width, extent.height);

  _swap_chain = {
      .depth         = depth,
      .multisampling = multisampling,
      .frame_buffers = A_DARRAY_SIZE(VkFramebuffer, _mem_render, image_count),
      .image_handles = A_DARRAY_SIZE(ImageHandle, _mem_render, image_count),
      .window        = window,
      .render_pass   = render_pass,
  };

  ASSERT_SUCCESS("failed to create swap chain!",
                 vkCreateSwapchainKHR(vulkan::_ctx.logical_device,
                                      &create_info,
                                      nullptr,
                                      &_swap_chain.swap_chain));

  vkGetSwapchainImagesKHR(vulkan::_ctx.logical_device,
                          _swap_chain.swap_chain,
                          &image_count,
                          nullptr);

  VkImage images[image_count];
  vkGetSwapchainImagesKHR(vulkan::_ctx.logical_device,
                          _swap_chain.swap_chain,
                          &image_count,
                          images);

  for (U32 i = 0; i < image_count; i++) {
    _swap_chain.image_handles._data[i] = vulkan::images::add(images[i], surface_format.format, 1);
  }

  _swap_chain.image_format = surface_format.format;
  _swap_chain.extent       = extent;

  for (U32 i = 0; i < image_count; i++) {
    vulkan::images::create_view(_swap_chain.image_handles._data[i], VK_IMAGE_ASPECT_COLOR_BIT);
  }
  create_framebuffers(render_pass);
}

void vulkan::swap_chain::cleanup() {
  vulkan::images::cleanup_view(vulkan::_swap_chain.multisampling);
  vulkan::images::cleanup(vulkan::_swap_chain.multisampling);
  vulkan::images::cleanup_view(vulkan::_swap_chain.depth);
  vulkan::images::cleanup(vulkan::_swap_chain.depth);

  for (U32 i = 0; i < vulkan::_swap_chain.frame_buffers._size; i++) {
    vkDestroyFramebuffer(vulkan::_ctx.logical_device,
                         vulkan::_swap_chain.frame_buffers._data[i],
                         nullptr);
    vulkan::_swap_chain.frame_buffers._data[i] = VK_NULL_HANDLE;
  }

  for (U32 i = 0; i < vulkan::_swap_chain.image_handles._size; i++) {
    vulkan::images::cleanup_view(vulkan::_swap_chain.image_handles._data[i]);
    vulkan::images::remove(vulkan::_swap_chain.image_handles._data[i]);
  }

  vkDestroySwapchainKHR(vulkan::_ctx.logical_device, vulkan::_swap_chain.swap_chain, nullptr);
}

void vulkan::swap_chain::recreate() {
  int width = 0, height = 0;
  SDL_GetWindowSize(vulkan::_swap_chain.window, &width, &height);
  while (width == 0 || height == 0) {
    SDL_GetWindowSize(vulkan::_swap_chain.window, &width, &height);
    SDL_Event e;
    SDL_PollEvent(&e);
  }

  vkDeviceWaitIdle(vulkan::_ctx.logical_device);

  cleanup();

  vulkan::swap_chain::create(vulkan::_swap_chain.render_pass, vulkan::_swap_chain.window);
}

vulkan::SwapChainSupport
vulkan::swap_chain::swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupport details{
      .capabilities  = VkSurfaceCapabilitiesKHR{},
      .formats       = S_DARRAY_EMPTY(VkSurfaceFormatKHR),
      .present_modes = S_DARRAY_EMPTY(VkPresentModeKHR),
  };

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  U32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  if (format_count != 0) {
    array::resize(details.formats, format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats._data);
  }

  U32 present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
  if (present_mode_count != 0) {
    array::resize(details.present_modes, present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                              surface,
                                              &present_mode_count,
                                              details.present_modes._data);
  }

  return details;
}
