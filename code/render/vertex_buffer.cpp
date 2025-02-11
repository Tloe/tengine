#include "vertex_buffer.h"
#include "arena.h"
#include "ds_array_static.h"
#include <vulkan/vulkan_core.h>

VkVertexInputBindingDescription render::vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};

  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

ds::StaticArray<VkVertexInputAttributeDescription, 3> render::vertex::getAttributeDescriptions(mem::Arena * scratch) {
  ds::StaticArray<VkVertexInputAttributeDescription, 3> attributeDescriptions = ds::array::init<VkVertexInputAttributeDescription, 3>(scratch);

  attributeDescriptions.data[0].binding = 0;
  attributeDescriptions.data[0].location = 0;
  attributeDescriptions.data[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions.data[0].offset = offsetof(Vertex, pos);

  attributeDescriptions.data[1].binding = 0;
  attributeDescriptions.data[1].location = 1;
  attributeDescriptions.data[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions.data[1].offset = offsetof(Vertex, color);

  attributeDescriptions.data[2].binding = 0;
  attributeDescriptions.data[2].location = 2;
  attributeDescriptions.data[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions.data[2].offset = offsetof(Vertex, texCoord);

  return attributeDescriptions;
}
