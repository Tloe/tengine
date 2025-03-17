#pragma once

#include "ds_array_dynamic.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

namespace vulkan {
  struct Vertex {
    glm::vec3 pos;
    glm::vec2 texture_pos;
  };

  bool operator==(const Vertex& lhs, const Vertex& rhs);

  namespace vertex {
    VkVertexInputBindingDescription                   binding_description();
    DynamicArray<VkVertexInputAttributeDescription> attribute_descriptions();
  }
}
