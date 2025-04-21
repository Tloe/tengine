#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/index_buffers.h"
#include "vulkan/vertex_buffers.h"

#include <cstdio>
#include <vulkan/vulkan_core.h>

struct Mesh {
  vulkan::VertexBufferHandle vertex_buffer;
  vulkan::IndexBufferHandle  index_buffer;
};

namespace meshes {
  MeshHandle create(const char* fpath);
  MeshHandle
  create(vulkan::VertexBufferHandle vertex_buffer, vulkan::IndexBufferHandle index_buffer);

  template <typename VertexT>
  MeshHandle create(const VertexT* vertices,
                    const U32      vertices_size,
                    const U32*     indices,
                    const U32      indices_size) {
    return create(vulkan::vertex_buffers::create(static_cast<const void*>(vertices), vertices_size * sizeof(VertexT)),
                  vulkan::index_buffers::create(indices, indices_size));
  }

  template <typename VertexT>
  MeshHandle create(DynamicArray<VertexT>& vertices, DynamicArray<U32>& indices) {
    return create(vulkan::vertex_buffers::create(vertices), vulkan::index_buffers::create(indices));
  }

  template <typename VertexT>
  MeshHandle create(DynamicArray<VertexT>& vertices) {
    return create(vulkan::vertex_buffers::create(vertices), vulkan::IndexBufferHandle());
  }

  void cleanup(MeshHandle handle);

  void draw(vulkan::CommandBufferHandle command_buffer, MeshHandle handle);
}
