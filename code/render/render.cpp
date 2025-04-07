#include "render.h"

#include "arena.h"
#include "ds_array_dynamic.h"
#include "handles.h"
#include "meshes.h"
#include "ui.h"
#include "vulkan/command_buffers.h"
#include "vulkan/context.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/render_pass.h"
#include "vulkan/swap_chain.h"
#include "vulkan/ubos.h"
#include "vulkan/vulkan.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace {
#ifdef NDEBUG
  const bool debug = false;
#else
  const bool debug = true;
#endif
  bool framebuffer_resized = false;

  vulkan::RenderPassHandle render_pass;

  const U32 MAX_FRAMES_IN_FLIGHT = 2;
  U32       current_frame        = 0;
  struct {
    U32                         vk_image_index;
    VkSemaphore                 image_available;
    VkSemaphore                 render_finished;
    VkFence                     in_flight_fence;
    vulkan::CommandBufferHandle command_buffer;
    ArenaHandle                 arena;
  } frames[MAX_FRAMES_IN_FLIGHT];

  void init_frames() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    char frame_str[] = "frame0";

    for (U8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      ASSERT_SUCCESS("failed to create semaphores!",
                     vkCreateSemaphore(vulkan::_ctx.logical_device,
                                       &semaphore_info,
                                       nullptr,
                                       &frames[i].image_available));

      ASSERT_SUCCESS("failed to create semaphores!",
                     vkCreateSemaphore(vulkan::_ctx.logical_device,
                                       &semaphore_info,
                                       nullptr,
                                       &frames[i].render_finished));

      ASSERT_SUCCESS("failed to create semaphores!",
                     vkCreateFence(vulkan::_ctx.logical_device,
                                   &fence_info,
                                   nullptr,
                                   &frames[i].in_flight_fence));

      frames[i].command_buffer = vulkan::command_buffers::create();

      frames[i].arena = arena::by_name(frame_str);
      snprintf(frame_str, 7, "frame%d", i);
    }
  }

  void cleanup_frames() {
    for (U8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(vulkan::_ctx.logical_device, frames[i].render_finished, nullptr);
      vkDestroySemaphore(vulkan::_ctx.logical_device, frames[i].image_available, nullptr);
      vkDestroyFence(vulkan::_ctx.logical_device, frames[i].in_flight_fence, nullptr);
    }
  }
}

void render::init(Settings settings, SDL_Window* sdl_window) {
  SET_SCRATCH(settings.memory_init_scratch);

  vulkan::context::init(sdl_window, debug);

  vulkan::command_buffers::init();

  render_pass = vulkan::render_pass::create();

  vulkan::swap_chain::create(render_pass, sdl_window);

  vulkan::ubos::init(settings.max_textures);

  for (U32 i = 0; i < settings.pipeline_settings_count; ++i) {
    vulkan::pipelines::create(settings.pipeline_settings[i], render_pass);
  }

  ui::init(sdl_window, render_pass);

  init_frames();

  arena::reset(arena::scratch());
}

void render::cleanup() {
  vulkan::swap_chain::cleanup();

  cleanup_frames();

  vulkan::ubos::cleanup();

  vulkan::pipelines::cleanup();

  vulkan::render_pass::cleanup(render_pass);

  vulkan::command_buffers::cleanup();
  vulkan::context::cleanup(debug);
}

void
render::bind_pipeline(vulkan::PipelineHandle pipeline) {
  auto frame = &frames[current_frame];

  vulkan::pipelines::bind(pipeline, frame->command_buffer);
  vulkan::ubos::bind_global_ubo(frame->command_buffer, pipeline);
  vulkan::ubos::bind_model_ubo(frame->command_buffer, pipeline); 
  vulkan::ubos::bind_textures(frame->command_buffer, pipeline); 
} 

void render::set_view_projection(glm::mat4& view, glm::mat4& proj) {
  vulkan::ubos::set_global_ubo(vulkan::ubos::GlobalUBO{
      .view = view,
      .proj = proj,
  });
}

void render::set_model(glm::mat4& model) {
  vulkan::ubos::set_model_ubo(vulkan::ubos::ModelUBO{
      .model         = model,
      .texture_index = 0,
  });
}

void render::draw_mesh(MeshHandle mesh) {
  meshes::draw(frames[current_frame].command_buffer, mesh);
}

void render::begin_frame() {
  auto frame = &frames[current_frame];

  vkWaitForFences(vulkan::_ctx.logical_device, 1, &frame->in_flight_fence, VK_TRUE, UINT64_MAX);

  arena::reset(frame->arena);

  auto result = vkAcquireNextImageKHR(vulkan::_ctx.logical_device,
                                      vulkan::_swap_chain.swap_chain,
                                      UINT64_MAX,
                                      frame->image_available,
                                      VK_NULL_HANDLE,
                                      &frame->vk_image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    vulkan::swap_chain::recreate();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    printf("failed to acquire swap chain image!");
    exit(0);
  }

  vkResetFences(vulkan::_ctx.logical_device, 1, &frame->in_flight_fence);

  vulkan::command_buffers::reset(frame->command_buffer);

  auto vk_command_buffer = *vulkan::command_buffers::begin(frame->command_buffer);

  vulkan::render_pass::begin(render_pass,
                             frame->command_buffer,
                             vulkan::_swap_chain.frame_buffers._data[frame->vk_image_index]);

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
  auto frame = &frames[current_frame];

  ui::draw_frame(frame->command_buffer);

  vulkan::render_pass::end(frame->command_buffer);

  ASSERT_SUCCESS("failed to record command buffer!",
                 vkEndCommandBuffer(*vulkan::command_buffers::buffer(frame->command_buffer)));

  VkSemaphore          wait_semaphores[]   = {frame->image_available};
  VkSemaphore          signal_semaphores[] = {frame->render_finished};
  VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submit_info{};
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount   = 1;
  submit_info.pWaitSemaphores      = wait_semaphores;
  submit_info.pWaitDstStageMask    = wait_stages;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = vulkan::command_buffers::buffer(frame->command_buffer);
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_semaphores;

  ASSERT_SUCCESS(
      "failed to submit draw command buffer!",
      vkQueueSubmit(vulkan::_ctx.graphics_queue, 1, &submit_info, frame->in_flight_fence));

  VkSwapchainKHR swap_chains[] = {vulkan::_swap_chain.swap_chain};

  VkPresentInfoKHR present_info{};
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;
  present_info.swapchainCount     = 1;
  present_info.pSwapchains        = swap_chains;
  present_info.pImageIndices      = &frame->vk_image_index;
  present_info.pResults           = nullptr;

  auto result = vkQueuePresentKHR(vulkan::_ctx.present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
    framebuffer_resized = false;
    vulkan::swap_chain::recreate();
  } else if (result != VK_SUCCESS) {
    printf("failed to present swap chain image!");
    exit(0);
  }

  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

  vkDeviceWaitIdle(vulkan::_ctx.logical_device);

  ui::cleanup_frame();
}

void render::resize_framebuffers() { framebuffer_resized = true; }
