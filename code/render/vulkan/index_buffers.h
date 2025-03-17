#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

namespace vulkan {
  namespace index_buffers {
    IndexBufferHandle create(const DynamicArray<U32>& indices);
    void              cleanup(IndexBufferHandle handle);

    U32 size(IndexBufferHandle handle);
  }
}
