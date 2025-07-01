#include "meshes.h"

#include "arena.h"
#include "defines.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "ds_sparse_array.h"
#include "handle.h"
#include "handles.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/glm_includes.h"
#include "vulkan/handles.h"
#include "vulkan/pipelines.h"
#include "vulkan/vertex.h"
#include "vulkan/vulkan_include.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace {
  struct Mesh {
    vulkan::VertexBufferHandle vertex_buffer;
    U32                        vertex_count       = 0;
    U32                        vertex_byte_offset = 0;
    vulkan::IndexBufferHandle  index_buffer;
    U32                        index_count       = 0;
    U32                        index_byte_offset = 0;
    MeshGPUConstants           gpu_constants;
  };

  ArenaHandle mem_render = arena::by_name("render");

  auto _meshes = sparse::init16<Mesh, MESH_COUNT, MESH_COUNT>(mem_render);
}

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

  auto vertices = S_DARRAY_EMPTY(vulkan::VertexTex);
  auto indices  = S_DARRAY_EMPTY(U32);

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
      .vertex_count  = vertices._size,
      .index_buffer  = vulkan::index_buffers::create(indices),
      .index_count   = indices._size,
  };

  MeshHandle mesh_handle = MeshHandle{.value = sparse::next_id(_meshes)};

  sparse::insert(_meshes, mesh_handle.value, mesh);

  return mesh_handle;
}

MeshHandle meshes::create(vulkan::VertexBufferHandle vertex_buffer,
                          vulkan::IndexBufferHandle  index_buffer,
                          U32                        vertex_count,
                          U32                        index_count) {
  Mesh mesh{
      .vertex_buffer = vertex_buffer,
      .vertex_count  = vertex_count,
      .index_buffer  = index_buffer,
      .index_count   = index_count,
  };

  MeshHandle mesh_handle = MeshHandle{.value = sparse::next_id(_meshes)};

  sparse::insert(_meshes, mesh_handle.value, mesh);

  return mesh_handle;
}

MeshHandle meshes::create(vulkan::VertexBufferHandle vertex_buffer,
                          vulkan::IndexBufferHandle  index_buffer,
                          U32                        vertex_count,
                          U32                        index_count,
                          U32                        vertex_byte_offset,
                          U32                        index_byte_offset) {
  Mesh mesh{
      .vertex_buffer      = vertex_buffer,
      .vertex_count       = vertex_count,
      .vertex_byte_offset = vertex_byte_offset,
      .index_buffer       = index_buffer,
      .index_count        = index_count,
      .index_byte_offset  = index_byte_offset,
  };

  MeshHandle mesh_handle = MeshHandle{.value = sparse::next_id(_meshes)};

  sparse::insert(_meshes, mesh_handle.value, mesh);

  return mesh_handle;
}

void meshes::set_constants(MeshHandle handle, glm::mat4 model, I32 texture_index) {
  auto mesh = sparse::value(_meshes, handle.value);

  mesh->gpu_constants = {model, texture_index};
}

void meshes::cleanup(MeshHandle handle, bool cleanup_buffers) {
  auto mesh = sparse::value(_meshes, handle.value);

  if (cleanup_buffers) {
    vulkan::vertex_buffers::cleanup(mesh->vertex_buffer);
    if (!handles::invalid(mesh->index_buffer)) {
      vulkan::index_buffers::cleanup(mesh->index_buffer);
    }
  }

  sparse::remove(_meshes, handle.value);
}

void meshes::draw(vulkan::PipelineHandle      pipeline,
                  vulkan::CommandBufferHandle command_buffer,
                  MeshHandle                  handle) {
  auto mesh = sparse::value(_meshes, handle.value);

  if (!mesh) {
    printf("MeshHandle not found");
    exit(0);
  }

  VkBuffer vertex_buffers[] = {
      *vulkan::buffers::vk_buffer(mesh->vertex_buffer),
  };

  auto vk_command_buffer = *vulkan::command_buffers::buffer(command_buffer);

  VkDeviceSize offsets[] = {mesh->vertex_byte_offset};

  vkCmdBindVertexBuffers(vk_command_buffer, 0, 1, vertex_buffers, offsets);

  if (!handles::invalid(mesh->index_buffer)) {
    auto index_buffer = *vulkan::buffers::vk_buffer(mesh->index_buffer);
    vkCmdBindIndexBuffer(vk_command_buffer,
                         index_buffer,
                         mesh->index_byte_offset,
                         VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(vk_command_buffer,
                       *vulkan::pipelines::layout(pipeline),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(MeshGPUConstants),
                       &mesh->gpu_constants);

    vkCmdDrawIndexed(vk_command_buffer, mesh->index_count, 1, 0, 0, 0);
  } else {
    vkCmdDraw(vk_command_buffer, mesh->vertex_count, 1, 0, 0);
  }
}
