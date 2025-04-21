#pragma once

#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"

#include <SDL3/SDL_video.h>
#include <vulkan/glm_includes.h>
#include <vulkan/vulkan_core.h>

namespace render {
  struct Settings {
    U32                          memory_init_scratch;
    U8                           max_frames;
    U16                          max_textures;
    U16                          width;
    U16                          height;
    U32                          ubo_count;
    vulkan::pipelines::Settings* pipeline_settings       = nullptr;
    U32                          pipeline_settings_count = 0;
  };

  void init(Settings settings, SDL_Window* sdl_window);
  void cleanup();

  void bind_pipeline(vulkan::PipelineHandle pipeline);

  void set_view_projection(glm::mat4& view, glm::mat4& proj);
  void set_model(glm::mat4& model);

  void draw_mesh(MeshHandle mesh);

  void begin_frame();
  void end_frame();

  void resize_framebuffers();
}
