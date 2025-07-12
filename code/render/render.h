#pragma once

#include "ds_array_dynamic.h"
#include "ds_string.h"
#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/ubos.h"

#include <SDL3/SDL_video.h>
#include <vulkan/glm_includes.h>

namespace render {
  struct MemorySettings {
    const char* name;
    U32         byte_size;
  };

  struct Settings {
    U8                     max_frames;
    U16                    max_textures;
    U16                    width;
    U16                    height;
    DynamicArray<String>   ui_fonts;
    vulkan::ubos::Settings ubo_settings;
  };

  void init(Settings settings, SDL_Window* sdl_window);
  void cleanup();

  vulkan::PipelineHandle create_pipeline(vulkan::pipelines::Settings settings);
  void                   bind_pipeline(vulkan::PipelineHandle pipeline);

  void set_view_projection(vulkan::UBOHandle ubo, const glm::mat4& view, const glm::mat4& proj);
  void set_model(vulkan::UBOHandle ubo, const glm::mat4& model, I32 texture_index = -1);

  void draw();
  void draw(MeshHandle mesh);

  void begin_frame();
  void end_frame();

  void resize_framebuffers();
}
