#include "vertex.h"

#include "ds_array_dynamic.h"

bool vulkan::operator==(const vulkan::Vertex& lhs, const vulkan::Vertex& rhs) {
  return lhs.pos == rhs.pos && lhs.texture_pos == rhs.texture_pos;
}

VkVertexInputBindingDescription vulkan::vertex::binding_description() {
  VkVertexInputBindingDescription description{};

  description.binding   = 0;
  description.stride    = sizeof(Vertex);
  description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return description;
}

DynamicArray<VkVertexInputAttributeDescription> vulkan::vertex::attribute_descriptions() {
  auto descriptions = array::init<VkVertexInputAttributeDescription>(arena::scratch(), 3);

  array::push_back(descriptions,
                   VkVertexInputAttributeDescription{
                       .location = 0,
                       .binding  = 0,
                       .format   = VK_FORMAT_R32G32B32_SFLOAT,
                       .offset   = offsetof(Vertex, pos),
                   });

  array::push_back(descriptions,
                   VkVertexInputAttributeDescription{
                       .location = 1,
                       .binding  = 0,
                       .format   = VK_FORMAT_R32G32_SFLOAT,
                       .offset   = offsetof(Vertex, texture_pos),
                   });

  return descriptions;
}
