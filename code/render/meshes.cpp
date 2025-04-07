#include "arena.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "handle.h"
#include "handles.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/glm_includes.h"
#include "vulkan/handles.h"
#include "vulkan/vertex.h"

#include <meshes.h>
#include <vulkan/vulkan_core.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace {
  ArenaHandle     mem_render_resource = arena::by_name("render_resources");
  HashMap16<Mesh> _meshes             = hashmap::init16<Mesh>(mem_render_resource);
  U16             next_handle_value   = 0;
} // namespace

namespace hashmap {
  template <>
  inline U64 hasher(const vulkan::VertexTex& vertex) {
    U64 hash = 0;
    hash ^= fnv_64a_buf((void*)&vertex.pos, sizeof(vertex.pos), FNV1A_64_INIT);
    hash ^= fnv_64a_buf((void*)&vertex.uv, sizeof(vertex.uv), hash);

    return hash;
  }

  template <>
  inline U64 hasher(const vulkan::Vertex2DColorTex& vertex) {
    U64 hash = 0;
    hash ^= fnv_64a_buf((void*)&vertex.pos, sizeof(vertex.pos), FNV1A_64_INIT);
    hash ^= fnv_64a_buf((void*)&vertex.color, sizeof(vertex.color), FNV1A_64_INIT);
    hash ^= fnv_64a_buf((void*)&vertex.uv, sizeof(vertex.uv), hash);

    return hash;
  }
}

MeshHandle meshes::create(const char* fpath) {
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fpath)) {
    printf("%s, %s", warn.c_str(), err.c_str());
    exit(0);
  }

  auto unique_vertices = hashmap::init<vulkan::VertexTex, vulkan::VertexTex{}, U32>(
      arena::scratch(),
      shapes.size(),
      hashmap::hasher<const vulkan::VertexTex&>);

  auto vertices = array::init<vulkan::VertexTex>(arena::scratch());
  auto indices  = array::init<U32>(arena::scratch());

  for (U32 shapes_i = 0; shapes_i < shapes.size(); ++shapes_i) {
    for (U32 indices_i = 0; indices_i < shapes[shapes_i].mesh.indices.size(); ++indices_i) {
      vulkan::VertexTex vertex{};
      auto              index = shapes[shapes_i].mesh.indices[indices_i];
      vertex.pos              = {attrib.vertices[3 * index.vertex_index + 0],
                                 attrib.vertices[3 * index.vertex_index + 1],
                                 attrib.vertices[3 * index.vertex_index + 2]};

      vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      if (!hashmap::contains(unique_vertices, vertex)) {
        hashmap::insert(unique_vertices, vertex, vertices._size);
        array::push_back(vertices, vertex);
      }

      array::push_back(indices, *hashmap::value(unique_vertices, vertex));
    }
  }

  Mesh mesh{
      .vertex_buffer = vulkan::vertex_buffers::create(vertices),
      .index_buffer  = vulkan::index_buffers::create(indices),
  };

  MeshHandle handle{.value = next_handle_value++};
  hashmap::insert(_meshes, handle.value, mesh);

  return handle;
}

MeshHandle
meshes::create(vulkan::VertexBufferHandle vertex_buffer, vulkan::IndexBufferHandle index_buffer) {
  Mesh mesh{
      .vertex_buffer = vertex_buffer,
      .index_buffer  = index_buffer,
  };

  MeshHandle handle{.value = next_handle_value++};
  hashmap::insert(_meshes, handle.value, mesh);

  return handle;
}

void meshes::cleanup(MeshHandle handle) {
  auto mesh = hashmap::value(_meshes, handle.value);

  vulkan::vertex_buffers::cleanup(mesh->vertex_buffer);
  vulkan::index_buffers::cleanup(mesh->index_buffer);
}

void meshes::draw(vulkan::CommandBufferHandle command_buffer, MeshHandle handle) {
  auto mesh = hashmap::value(_meshes, handle.value);

  VkBuffer vertex_buffers[] = {
      *vulkan::buffers::buffer(mesh->vertex_buffer),
  };

  auto vk_command_buffer = *vulkan::command_buffers::buffer(command_buffer);

  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, vertex_buffers, offsets);

  if (!handle::invalid(mesh->index_buffer)) {
    auto index_buffer = *vulkan::buffers::buffer(mesh->index_buffer);
    vkCmdBindIndexBuffer(vk_command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(vk_command_buffer,
                     vulkan::index_buffers::size(mesh->index_buffer),
                     1,
                     0,
                     0,
                     0);
  } else {
    vkCmdDraw(vk_command_buffer,
              vulkan::vertex_buffers::vertex_count(mesh->vertex_buffer),
              1,
              0,
              0);
  }
}
