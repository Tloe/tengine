#pragma once

#include "ds_array_dynamic.h"
#include "vulkan/handles.h"

#include "vulkan_include.h"

namespace vulkan {

  namespace vertex_buffers {
    VertexBufferHandle create(const void* vertices, VkDeviceSize byte_size);
    VertexBufferHandle create(VkDeviceSize byte_size);
    template <typename VertexT>
    VertexBufferHandle create(const DynamicArray<VertexT>& vertices) {
      return create(vertices._data, vertices._size * sizeof(VertexT));
    }

    void cleanup(VertexBufferHandle handle);
  }
}
