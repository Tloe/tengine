#pragma once

#include "ds_array_dynamic.h"
#include "handles.h"

#include "vulkan_include.h"

namespace vulkan {
  namespace index_buffers {
    IndexBufferHandle create(const DynamicArray<U32>& indices);
    IndexBufferHandle create(VkDeviceSize byte_size);
    IndexBufferHandle create(const U32* indices, VkDeviceSize byte_size);

    void              cleanup(IndexBufferHandle handle);
  }
}
