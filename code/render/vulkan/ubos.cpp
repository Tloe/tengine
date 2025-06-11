#include "ubos.h"

#include "buffers.h"
#include "context.h"
#include "ds_array_static.h"
#include "ds_hashmap.h"
#include "handle.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan/pipelines.h"
#include "vulkan/samplers.h"
#include "vulkan/textures.h"
#include "vulkan/vulkan.h"

#include <cstdio>
#include <vulkan/vulkan_core.h>

namespace {
  struct UBOData {
    U8                    set_index;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet       descriptor_set;
    vulkan::BufferHandle  buffer;
    U32                   memory_size;
    void*                 memory;
  };

  auto mem_render = arena::by_name("render");

  handles::Allocator<vulkan::UBOHandle, U8, 16> _handles;

  VkDescriptorPool _pool;

  auto _ubo_datas       = hashmap::init8<UBOData>(mem_render);
  auto _texture_indexes = hashmap::init16<U16>(mem_render);

  vulkan::UBOHandle init_ubo(VkDescriptorType type, U32 descriptor_count) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = type;
    binding.descriptorCount = descriptor_count;
    binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = &binding;

    auto ubo = handles::next(_handles);

    auto ubo_data = hashmap::insert(_ubo_datas, ubo.value);

    ASSERT_SUCCESS("failed to create ubo descriptor set layout!",
                   vkCreateDescriptorSetLayout(vulkan::_ctx.logical_device,
                                               &create_info,
                                               nullptr,
                                               &ubo_data->descriptor_set_layout));

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = _pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &ubo_data->descriptor_set_layout;

    vkAllocateDescriptorSets(vulkan::_ctx.logical_device, &alloc_info, &ubo_data->descriptor_set);

    return ubo;
  }
}

void vulkan::ubos::init(Settings settings) {
  VkDescriptorPoolSize pool_sizes[2];
  pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = settings.ubo_count;
  pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = settings.max_textures;

  VkDescriptorPoolCreateInfo pool_create_info{};
  pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.pPoolSizes    = pool_sizes;
  pool_create_info.poolSizeCount = array::size(pool_sizes);
  pool_create_info.maxSets       = settings.ubo_count + settings.texture_set_count;

  ASSERT_SUCCESS(
      "failed to create descriptor pool!",
      vkCreateDescriptorPool(vulkan::_ctx.logical_device, &pool_create_info, nullptr, &_pool));
}

void vulkan::ubos::cleanup() {
  vkDestroyDescriptorPool(vulkan::_ctx.logical_device, _pool, nullptr);
}

vulkan::UBOHandle vulkan::ubos::create_ubo(U8 set_index, U32 byte_size, U32 descriptor_count) {
  auto ubo = init_ubo(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptor_count);

  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);

  ubo_data->set_index = set_index;

  ubo_data->buffer = vulkan::buffers::create(byte_size,
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkDescriptorBufferInfo buffer_info{};
  buffer_info.buffer = *vulkan::buffers::vk_buffer(ubo_data->buffer);
  buffer_info.offset = 0;
  buffer_info.range  = byte_size;

  VkWriteDescriptorSet write{};
  write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet          = ubo_data->descriptor_set;
  write.dstBinding      = 0;
  write.descriptorCount = 1;
  write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.pBufferInfo     = &buffer_info;

  vkUpdateDescriptorSets(vulkan::_ctx.logical_device, 1, &write, 0, nullptr);

  vkMapMemory(vulkan::_ctx.logical_device,
              *vulkan::buffers::vk_memory(ubo_data->buffer),
              0,
              byte_size,
              0,
              &ubo_data->memory);

  ubo_data->memory_size = byte_size;

  return ubo;
}

vulkan::UBOHandle vulkan::ubos::create_texture_set(U8 set_index, U32 texture_count) {
  auto ubo            = init_ubo(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture_count);
  auto ubo_data       = hashmap::value(_ubo_datas, ubo.value);
  ubo_data->set_index = set_index;

  return ubo;
}

void vulkan::ubos::cleanup(UBOHandle ubo) {
  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);

  if (!handles::invalid(ubo_data->buffer)) {
    vulkan::buffers::cleanup(ubo_data->buffer);
  }

  vkDestroyDescriptorSetLayout(vulkan::_ctx.logical_device,
                               ubo_data->descriptor_set_layout,
                               nullptr);

  hashmap::erase(_ubo_datas, ubo.value);

  handles::free(_handles, ubo);
}

void vulkan::ubos::set_ubo(UBOHandle ubo, void* data) {
  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);

  memcpy(ubo_data->memory, data, ubo_data->memory_size);
}

void vulkan::ubos::set_textures(UBOHandle ubo, DynamicArray<vulkan::TextureHandle> textures) {
  VkDescriptorImageInfo image_infos[textures._size];

  for (U32 i = 0; i < textures._size; ++i) {
    image_infos[i].imageView   = *images::vk_view(textures._data[i]);
    image_infos[i].sampler     = *samplers::sampler(textures::sampler(textures._data[i]));
    image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hashmap::insert(_texture_indexes, textures._data[i].value, i);
  }

  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);

  VkWriteDescriptorSet texture_write{};
  texture_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  texture_write.dstSet          = ubo_data->descriptor_set;
  texture_write.dstBinding      = 0;
  texture_write.descriptorCount = textures._size;
  texture_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture_write.pImageInfo      = image_infos;

  vkUpdateDescriptorSets(vulkan::_ctx.logical_device, 1, &texture_write, 0, nullptr);
}

U32 vulkan::ubos::texture_index(vulkan::TextureHandle texture) {
  return *hashmap::value(_texture_indexes, texture.value);
}

void
vulkan::ubos::bind(UBOHandle ubo, CommandBufferHandle command_buffer, PipelineHandle pipeline) {
  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);

  vkCmdBindDescriptorSets(*command_buffers::buffer(command_buffer),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipelines::layout(pipeline),
                          ubo_data->set_index,
                          1,
                          &ubo_data->descriptor_set,
                          0,
                          nullptr);
}

VkDescriptorSetLayout vulkan::ubos::descriptor_set_layout(UBOHandle ubo) {
  auto ubo_data = hashmap::value(_ubo_datas, ubo.value);
  return ubo_data->descriptor_set_layout;
}
