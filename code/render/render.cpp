#include "render.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "frame.h"
#include "handles.h"
#include "meshes.h"
#include "ui.h"
#include "vulkan/command_buffers.h"
#include "vulkan/common.h"
#include "vulkan/context.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/render_pass.h"
#include "vulkan/swap_chain.h"
#include "vulkan/ubos.h"
#include "vulkan/vulkan_include.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
#ifdef NDEBUG
  const bool debug = false;
#else
  const bool debug = true;
#endif
  bool framebuffer_resized = false;

  vulkan::RenderPassHandle render_pass;
}

void render::init(Settings settings, SDL_Window* sdl_window) {
  vulkan::context::init(sdl_window, debug);

  vulkan::command_buffers::init();

  render_pass = vulkan::render_pass::create();

  vulkan::swap_chain::create(render_pass, sdl_window);

  vulkan::ubos::init(settings.ubo_settings);

  ui::init(sdl_window, render_pass, settings.ui_fonts);

  frame::init(vulkan::_ctx.logical_device);

  arena::reset(arena::scratch());
}

void render::cleanup() {
  vulkan::swap_chain::cleanup();

  frame::cleanup(vulkan::_ctx.logical_device);

  ui::cleanup();

  vulkan::ubos::cleanup();

  vulkan::pipelines::cleanup();

  vulkan::render_pass::cleanup(render_pass);

  vulkan::command_buffers::cleanup();
  vulkan::context::cleanup(debug);
}

vulkan::PipelineHandle render::create_pipeline(vulkan::pipelines::Settings settings) {
  return vulkan::pipelines::create(render_pass, settings);
}

void render::bind_pipeline(vulkan::PipelineHandle pipeline) {
  vulkan::pipelines::bind(pipeline, frame::current->command_buffer);

  frame::current->current_pipeline = pipeline;
}

void
render::set_view_projection(vulkan::UBOHandle ubo, const glm::mat4& view, const glm::mat4& proj) {
  auto global_ubo = vulkan::GlobalUBO{
      .view = view,
      .proj = proj,
  };

  
  memcpy(vulkan::ubos::mapped(ubo), &global_ubo, sizeof(global_ubo));
}

void render::draw() {
  // TODO : move to vulkan
  vkCmdDraw(*vulkan::command_buffers::buffer(frame::current->command_buffer), 3, 1, 0, 0);
}

void render::draw(MeshHandle mesh) {
  meshes::draw(frame::current->current_pipeline, frame::current->command_buffer, mesh);
}

void render::begin_frame() {
  vkWaitForFences(vulkan::_ctx.logical_device, 1, &frame::current->fence, VK_TRUE, UINT64_MAX);

  arena::set_frame(frame::current->index);
  arena::reset(frame::current->arena);

  auto result = vkAcquireNextImageKHR(vulkan::_ctx.logical_device,
                                      vulkan::_swap_chain.swap_chain,
                                      UINT64_MAX,
                                      frame::current->image_available,
                                      VK_NULL_HANDLE,
                                      &frame::current->vk_image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    vulkan::swap_chain::recreate();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    printf("failed to acquire swap chain image!");
    exit(0);
  }

  vkResetFences(vulkan::_ctx.logical_device, 1, &frame::current->fence);

  vulkan::command_buffers::wait(frame::current->command_buffer);
  vulkan::command_buffers::reset(frame::current->command_buffer);
  auto vk_command_buffer = *vulkan::command_buffers::begin(frame::current->command_buffer);

  vulkan::render_pass::begin(
      render_pass,
      frame::current->command_buffer,
      vulkan::_swap_chain.frame_buffers._data[frame::current->vk_image_index]);

  VkViewport viewport{
      .x        = 0.0f,
      .y        = 0.0f,
      .width    = static_cast<F32>(vulkan::_swap_chain.extent.width),
      .height   = static_cast<F32>(vulkan::_swap_chain.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);

  VkRect2D scissor{
      .offset = {0, 0},
      .extent = vulkan::_swap_chain.extent,
  };
  vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);
}

void render::end_frame() {
  ui::draw_frame();

  vulkan::render_pass::end(frame::current->command_buffer);

  ASSERT_SUCCESS(
      "failed to record command buffer!",
      vkEndCommandBuffer(*vulkan::command_buffers::buffer(frame::current->command_buffer)));

  VkSemaphore          wait_semaphores[]   = {frame::current->image_available};
  VkSemaphore          signal_semaphores[] = {frame::current->render_finished};
  VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores    = wait_semaphores;
  submit_info.pWaitDstStageMask  = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = vulkan::command_buffers::buffer(frame::current->command_buffer);
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_semaphores;

  ASSERT_SUCCESS(
      "failed to submit draw command buffer!",
      vkQueueSubmit(vulkan::_ctx.graphics_queue, 1, &submit_info, frame::current->fence));

  VkSwapchainKHR swap_chains[] = {vulkan::_swap_chain.swap_chain};

  VkPresentInfoKHR present_info{};
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;
  present_info.swapchainCount     = 1;
  present_info.pSwapchains        = swap_chains;
  present_info.pImageIndices      = &frame::current->vk_image_index;
  present_info.pResults           = nullptr;

  auto result = vkQueuePresentKHR(vulkan::_ctx.present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
    framebuffer_resized = false;
    vulkan::swap_chain::recreate();
  } else if (result != VK_SUCCESS) {
    printf("failed to present swap chain image!");
    exit(0);
  }

  vkDeviceWaitIdle(vulkan::_ctx.logical_device);

  frame::current = frame::next();
}

void render::resize_framebuffers() { framebuffer_resized = true; }
