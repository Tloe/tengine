#pragma once

#include "handles.h"
#include "vulkan_include.h"


namespace vulkan {
  namespace ssbo {
    SSBOBufferHandle create(VkDeviceSize byte_size);
    void       cleanup(SSBOBufferHandle handle);
  }
}
