#pragma once

#include "ds_array_dynamic.h"
#include "vulkan/handles.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {

  namespace vertex_buffers {
    VertexBufferHandle create(const void* vertices, VkDeviceSize byte_size);
    template <typename VertexT>
    VertexBufferHandle create(const DynamicArray<VertexT>& vertices) {
      return create(vertices._data, vertices._size * sizeof(VertexT));
    }

    // void update(VertexBufferHandle vertex_buffer, const void* vertices, VkDeviceSize byte_size);
    // template <typename VertexT>
    // void update(VertexBufferHandle vertex_buffer, const DynamicArray<VertexT>& vertices) {
    //   return update(vertex_buffer, vertices._data, sizeof(vertices._data[0]) * vertices._size);
    // }

    void cleanup(VertexBufferHandle handle);

    U32 vertex_count(VertexBufferHandle handle);
  }
}
