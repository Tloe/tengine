#pragma once

#include "ds_array_dynamic.h"
#include "vulkan/handles.h"
#include "vulkan/vertex.h"

#include <vulkan/vulkan_core.h>

namespace vulkan {

  namespace vertex_buffers {
    VertexBufferHandle create(const DynamicArray<Vertex>& vertices);
    void               cleanup(VertexBufferHandle handle);

    U32 vertex_count(VertexBufferHandle handle);
  }
}
