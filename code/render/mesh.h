#pragma once

#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/vertex.h"

#include <vulkan/vulkan_core.h>

namespace render {
  struct Mesh {
    vulkan::VertexBufferHandle vertex_buffer;
    vulkan::IndexBufferHandle  index_buffer;
  };

  namespace meshes {
    MeshHandle create(const char* fpath);
    MeshHandle create(DynamicArray<vulkan::Vertex>& vertices, DynamicArray<U32>& indices);

    void cleanup(render::MeshHandle handle);

    void draw(VkCommandBuffer cmd_buffer, MeshHandle handle);
  }
}
