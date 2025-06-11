#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

namespace vulkan {
  namespace index_buffers {
    IndexBufferHandle create(const DynamicArray<U32>& indices);
    IndexBufferHandle create(const U32* indices, U32 size);
    void              cleanup(IndexBufferHandle handle);
  }
}
