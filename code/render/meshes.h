#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"
#include "vulkan/handles.h"
#include "vulkan/index_buffers.h"
#include "vulkan/vertex_buffers.h"

#include <cstdio>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

struct alignas(16) MeshGPUConstants {
  glm::mat4 model               = glm::mat4(1.0f); // 64 bytes
  alignas(16) I32 texture_index = -1;              // offset 64
  I32 _pad[3];
};

namespace meshes {
  MeshHandle create(ArenaHandle arena, const char* fpath);
  MeshHandle create(ArenaHandle                arena,
                    vulkan::VertexBufferHandle vertex_buffer,
                    vulkan::IndexBufferHandle  index_buffer,
                    U32                        vertex_count,
                    U32                        index_count);

  void set_constants(MeshHandle handle, glm::mat4 model, I32 texture_index = -1);

  void cleanup(MeshHandle handle);

  void draw(vulkan::PipelineHandle      pipeline,
            vulkan::CommandBufferHandle command_buffer,
            MeshHandle                  handle);

  void reset_arena(ArenaHandle arena);

  // templated
  template <typename VertexT>
  MeshHandle create(ArenaHandle    arena,
                    const VertexT* vertices,
                    const U32      vertices_size,
                    const U32*     indices,
                    const U32      indices_size) {
    return create(arena,
                  vulkan::vertex_buffers::create(static_cast<const void*>(vertices),
                                                 vertices_size * sizeof(VertexT)),
                  vulkan::index_buffers::create(indices, indices_size),
                  vertices_size,
                  indices_size);
  }

  template <typename VertexT>
  MeshHandle create(ArenaHandle            arena,
                    DynamicArray<VertexT>& vertices,
                    DynamicArray<U32>&     indices = S_DARRAY_EMPTY(VertexT)) {
    if (indices._size == 0) {
      return create(arena,
                    vulkan::vertex_buffers::create(vertices),
                    vulkan::IndexBufferHandle(),
                    vertices._size,
                    0);
    }

    return create(arena,
                  vulkan::vertex_buffers::create(vertices),
                  vulkan::index_buffers::create(indices),
                  vertices._size,
                  indices._size);
  }

  // template <typename VertexT>
  // MeshHandle create(DynamicArray<VertexT>& vertices) {
  //   return create(vulkan::vertex_buffers::create(vertices), vulkan::IndexBufferHandle());
  // }
}
