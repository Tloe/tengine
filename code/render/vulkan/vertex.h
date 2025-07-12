#pragma once

#include "glm_includes.h"

#include <cstddef>
#include "vulkan_include.h"

namespace vulkan {
  //Vertex
  struct Vertex2D {
    glm::vec2 pos;
  };

  inline bool operator==(const Vertex2D& lhs, const Vertex2D& rhs) {
    return lhs.pos == rhs.pos;
  }

  constexpr VkVertexInputAttributeDescription VERTEX_ATTRIBUTE_DESC[] = {
      {
          .location = 0,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(Vertex2D, pos),
      },
  };

  constexpr VkVertexInputBindingDescription VERTEX_BINDING_DESC = {
      .binding   = 0,
      .stride    = sizeof(Vertex2D),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  //VertexTex
  struct VertexTex {
    glm::vec3 pos;
    glm::vec2 uv;
  };

  inline bool operator==(const VertexTex& lhs, const VertexTex& rhs) {
    return lhs.pos == rhs.pos && lhs.uv == rhs.uv;
  }

  constexpr VkVertexInputAttributeDescription VERTEX_TEX_ATTRIBUTE_DESC[] = {
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

  //Vertex2DColorTex
  struct Vertex2DColorTex {
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 uv;
  };

  inline bool operator==(const Vertex2DColorTex& lhs, const Vertex2DColorTex& rhs) {
    return lhs.pos == rhs.pos && lhs.color == rhs.color && lhs.uv == rhs.uv;
  }

  constexpr VkVertexInputAttributeDescription VERTEX2D_COLOR_TEX_ATTRIBUTE_DESC[] = {
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
      .stride    = sizeof(Vertex2DColorTex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
}
