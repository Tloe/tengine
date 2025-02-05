#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <functional>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "ds_array_static.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace render {
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex &other) const {
      return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
  };

  namespace vertex {
    VkVertexInputBindingDescription getBindingDescription();
    ds::StaticArray<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
  }
}

namespace std {
  template <> struct hash<render::Vertex> {
    size_t operator()(render::Vertex const &vertex) const {
      return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
             (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

#endif
