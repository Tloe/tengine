#pragma once

#include "glm_includes.h"

#include <cstddef>
#include <vulkan/vulkan_core.h>

namespace vulkan {
  struct VertexTex {
    glm::vec3 pos;
    glm::vec2 uv;
  };

  inline bool operator==(const VertexTex& lhs, const VertexTex& rhs) {
    return lhs.pos == rhs.pos && lhs.uv == rhs.uv;
  }

  struct Vertex2DColorTex {
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 uv;
  };

  inline bool operator==(const Vertex2DColorTex& lhs, const Vertex2DColorTex& rhs) {
    return lhs.pos == rhs.pos && lhs.color == rhs.color && lhs.uv == rhs.uv;
  }

  constexpr VkVertexInputAttributeDescription VERTEX_TEX_ATTRIBUTE_DESC[2] = {
      {
          .location = 0,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(VertexTex, pos),
      },
      {
          .location = 1,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32_SFLOAT,
          .offset   = offsetof(VertexTex, uv),
      },
  };

  constexpr VkVertexInputBindingDescription VERTEX_TEX_BINDING_DESC = {
      .binding   = 0,
      .stride    = sizeof(VertexTex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  constexpr VkVertexInputAttributeDescription VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC[3] = {
      {
          .location = 0,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32_SFLOAT,
          .offset   = offsetof(Vertex2DColorTex, pos),
      },
      {
          .location = 1,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
          .offset   = offsetof(Vertex2DColorTex, color),
      },
      {
          .location = 2,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32_SFLOAT,
          .offset   = offsetof(Vertex2DColorTex, uv),
      },
  };

  constexpr VkVertexInputBindingDescription VERTEX2D_COLOR_TEX_BINDING_DESC = {
      .binding   = 0,
      .stride    = sizeof(VertexTex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
}
