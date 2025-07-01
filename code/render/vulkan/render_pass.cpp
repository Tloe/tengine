#include "render_pass.h"

#include "arena.h"
#include "command_buffers.h"
#include "context.h"
#include "device.h"
#include "ds_array_static.h"
#include "ds_hashmap.h"
#include "swap_chain.h"
#include "vulkan/common.h"
#include "vulkan/swap_chain.h"
#include "vulkan_include.h"

#include <cstdio>

namespace {
  HashMap16<VkRenderPass> render_passes = hashmap::init16<VkRenderPass>(arena::by_name("render"));
  U16                     next_render_pass_handle = 0;
}

vulkan::RenderPassHandle vulkan::render_pass::create() {
  auto surface_format = vulkan::context::surface_format();

  VkAttachmentDescription color_attachment{};
  color_attachment.format         = surface_format.format;
  color_attachment.samples        = vulkan::_ctx.max_msaa_samples;
  color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription color_attachment_resolve{};
  color_attachment_resolve.format         = surface_format.format;
  color_attachment_resolve.samples        = VK_SAMPLE_COUNT_1_BIT;
  color_attachment_resolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_resolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_resolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_resolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth_attachment{};
  depth_attachment.format         = vulkan::device::find_depth_format(vulkan::_ctx.physical_device);
  depth_attachment.samples        = vulkan::_ctx.max_msaa_samples;
  depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attachment_resolve_ref{};
  color_attachment_resolve_ref.attachment = 2;
  color_attachment_resolve_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;
  subpass.pResolveAttachments     = &color_attachment_resolve_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[] = {color_attachment,
                                           depth_attachment,
                                           color_attachment_resolve};

  VkRenderPassCreateInfo create_info{};
  create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.attachmentCount = array::size(attachments);
  create_info.pAttachments    = attachments;
  create_info.subpassCount    = 1;
  create_info.pSubpasses      = &subpass;
  create_info.dependencyCount = 1;
  create_info.pDependencies   = &dependency;

  RenderPassHandle handle{.value = next_render_pass_handle++};

  auto render_pass = hashmap::insert(::render_passes, handle.value, VkRenderPass{});

  ASSERT_SUCCESS(
      "failed to create render pass!",
      vkCreateRenderPass(vulkan::_ctx.logical_device, &create_info, nullptr, render_pass));

  return handle;
}

void vulkan::render_pass::cleanup(RenderPassHandle render_pass) {
  vkDestroyRenderPass(vulkan::_ctx.logical_device,
                      *hashmap::value(::render_passes, render_pass.value),
                      nullptr);
}

void vulkan::render_pass::begin(RenderPassHandle    render_pass,
                                CommandBufferHandle command_buffer,
                                VkFramebuffer       framebuffer) {
  VkClearValue clear_values[2] = {{}};
  clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clear_values[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = *render_pass::render_pass(render_pass);
  render_pass_info.framebuffer       = framebuffer;
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = vulkan::_swap_chain.extent;
  render_pass_info.clearValueCount   = array::size(clear_values);
  render_pass_info.pClearValues      = clear_values;

  vkCmdBeginRenderPass(*command_buffers::buffer(command_buffer),
                       &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void vulkan::render_pass::end(CommandBufferHandle command_buffer_handle) {
  vkCmdEndRenderPass(*command_buffers::buffer(command_buffer_handle));
}

VkRenderPass* vulkan::render_pass::render_pass(RenderPassHandle render_pass) {
  return hashmap::value(::render_passes, render_pass.value);
}
