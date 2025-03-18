#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"

#include <SDL3/SDL_video.h>
#include <vulkan/glm_includes.h>
#include <vulkan/vulkan_core.h>

namespace render {
  struct Settings {
    U32 memory_init_scratch;
    U8  max_frames;
    U16 max_textures;
    U16 width;
    U16 height;
  };

  struct Renderer {
    vulkan::RenderPassHandle render_pass;
    SDL_Window*              window;
    DynamicArray<MeshHandle> meshes;
  };

  Renderer init(Settings settings);
  void     cleanup(Renderer& renderer);

  void set_view_projection(glm::mat4& view, glm::mat4& proj);
  void set_model(glm::mat4& model);

  /* void bind_texture(vulkan::TextureHandle texture); */
  void draw_mesh(MeshHandle mesh);

  void begin_frame(Renderer& renderer);
  void end_frame(Renderer& renderer);

  void resize_framebuffers();
}
