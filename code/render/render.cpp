#include "render.h"

#include "arena.h"
#include "handles.h"
#include "mesh.h"
#include "vulkan/command_buffers.h"
#include "vulkan/context.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/render_pass.h"
#include "vulkan/swap_chain.h"
#include "vulkan/ubos.h"
#include "vulkan/vertex.h"
#include "vulkan/vulkan.h"
#include "vulkan/window.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace {
#ifdef NDEBUG
  const bool debug = false;
#else
  const bool debug = true;
#endif
  bool                   framebuffer_resized = false;
  vulkan::PipelineHandle pipeline;

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

render::Renderer render::init(Settings settings) {
  SET_SCRATCH(settings.memory_init_scratch);

  Renderer renderer;
  renderer.window = vulkan::window::init(settings.width, settings.height);

  vulkan::context::init(renderer.window, debug);

  vulkan::command_buffers::init();

  renderer.render_pass = vulkan::render_pass::create();

  vulkan::swap_chain::create(renderer.render_pass, renderer.window);

  vulkan::ubos::init(settings.max_textures);

  pipeline = vulkan::pipelines::create(vulkan::pipelines::Settings{
      .vertex_shader_fpath    = "vert.spv",
      .fragment_shader_fpath  = "frag.spv",
      .binding_description    = vulkan::vertex::binding_description(),
      .attribute_descriptions = vulkan::vertex::attribute_descriptions(),
      .render_pass            = renderer.render_pass,
  });

  init_frames();

  arena::reset(arena::scratch());

  return renderer;
}

void render::cleanup(render::Renderer& renderer) {
  vulkan::swap_chain::cleanup();

  cleanup_frames();

  vulkan::ubos::cleanup();
  vulkan::pipelines::cleanup(pipeline);

  vulkan::render_pass::cleanup(renderer.render_pass);

  vulkan::command_buffers::cleanup();
  vulkan::context::cleanup(debug);
  vulkan::window::cleanup(renderer.window);

  SDL_Quit();
}

void render::set_view_projection(glm::mat4& view, glm::mat4& proj) {
  vulkan::ubos::set_global_ubo(vulkan::ubos::GlobalUBO{
      .view = view,
      .proj = proj,
  });

  vulkan::ubos::bind_global_ubo(frames[current_frame].command_buffer, pipeline);
}

void render::set_model(glm::mat4& model) {
  vulkan::ubos::set_model_ubo(vulkan::ubos::ModelUBO{
      .model         = model,
      .texture_index = 0,
  });

  vulkan::ubos::bind_model_ubo(frames[current_frame].command_buffer, pipeline);
}

void render::draw_mesh(MeshHandle mesh) {
  meshes::draw(*vulkan::command_buffers::buffer(frames[current_frame].command_buffer), mesh);
}

bool render::check_events(Renderer& renderer) {
  bool quit = false;

  SDL_Event e;
  SDL_zero(e);
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_EVENT_QUIT: {
        quit = true;
        break;
      }
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        framebuffer_resized = true;
      }
    }
  }

  return quit;
}

void render::begin_frame(Renderer& renderer) {
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

  vulkan::render_pass::begin(renderer.render_pass,
                             frame->command_buffer,
                             vulkan::_swap_chain.frame_buffers._data[frame->vk_image_index]);

  vulkan::pipelines::bind(pipeline, frame->command_buffer);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<F32>(vulkan::_swap_chain.extent.width);
  viewport.height   = static_cast<F32>(vulkan::_swap_chain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = vulkan::_swap_chain.extent;
  vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);

  vulkan::ubos::bind_textures(frame->command_buffer, pipeline);
}

void render::end_frame(Renderer& renderer) {
  auto frame = &frames[current_frame];

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

  ASSERT_SUCCESS("failed to submit draw command buffer!",
                 vkQueueSubmit(vulkan::_ctx.graphics_queue, 1, &submit_info, frame->in_flight_fence));

  VkSwapchainKHR swap_chains[] = {vulkan::_swap_chain.swap_chain};

  VkPresentInfoKHR present_info{};
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;
  present_info.swapchainCount     = 1;
  present_info.pSwapchains        = swap_chains;
  present_info.pImageIndices      = &frame->vk_image_index;
  present_info.pResults           = nullptr; // Optional

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
}
