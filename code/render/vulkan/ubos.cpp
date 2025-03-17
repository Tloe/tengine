#include "ubos.h"

#include "buffers.h"
#include "context.h"
#include "ds_array_static.h"
#include "vulkan/buffers.h"
#include "vulkan/command_buffers.h"
#include "vulkan/handles.h"
#include "vulkan/images.h"
#include "vulkan/pipelines.h"
#include "vulkan/samplers.h"
#include "vulkan/textures.h"
#include "vulkan/vulkan.h"

#include <cstdio>
#include <iostream>
#include <vulkan/vulkan_core.h>

namespace {
  enum UBOS {
    GLOBAL_UBO,
    MODEL_UBO,
    TEXTURE_UBO,
    COUNT,
  };

  VkDescriptorPool      pool;
  VkDescriptorSetLayout layouts[UBOS::COUNT];
  VkDescriptorSet       sets[UBOS::COUNT];
  vulkan::BufferHandle  buffers[UBOS::COUNT];
  void*                 memory_ptrs[2];

  void init_uniform_buffer(UBOS type, VkDeviceSize mem_byte_size) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = &binding;

    ASSERT_SUCCESS("failed to create ubo descriptor set layout!",
                   vkCreateDescriptorSetLayout(vulkan::_ctx.logical_device,
                                               &create_info,
                                               nullptr,
                                               &layouts[type]));

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &layouts[type];

    vkAllocateDescriptorSets(vulkan::_ctx.logical_device, &alloc_info, &sets[type]);

    buffers[type] = vulkan::buffers::create(mem_byte_size,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = *vulkan::buffers::buffer(buffers[type]);
    buffer_info.offset = 0;
    buffer_info.range  = mem_byte_size;

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = sets[type];
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo     = &buffer_info;

    vkUpdateDescriptorSets(vulkan::_ctx.logical_device, 1, &write, 0, nullptr);

    vkMapMemory(vulkan::_ctx.logical_device,
                *vulkan::buffers::memory(buffers[type]),
                0,
                mem_byte_size,
                0,
                &memory_ptrs[type]);
  }

  void init_textures_ubo(U16 max_textures) {
    VkDescriptorSetLayoutBinding texture_binding{};
    texture_binding.binding         = 0;
    texture_binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_binding.descriptorCount = max_textures;
    texture_binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texture_layout_create_info{};
    texture_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_layout_create_info.bindingCount = 1;
    texture_layout_create_info.pBindings    = &texture_binding;

    ASSERT_SUCCESS("failed to create global ubo descriptor set layout!",
                   vkCreateDescriptorSetLayout(vulkan::_ctx.logical_device,
                                               &texture_layout_create_info,
                                               nullptr,
                                               &::layouts[UBOS::TEXTURE_UBO]));

    VkDescriptorSetAllocateInfo bindlessAllocInfo{};
    bindlessAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    bindlessAllocInfo.descriptorPool     = pool;
    bindlessAllocInfo.descriptorSetCount = 1;
    bindlessAllocInfo.pSetLayouts        = &layouts[UBOS::TEXTURE_UBO];

    vkAllocateDescriptorSets(vulkan::_ctx.logical_device,
                             &bindlessAllocInfo,
                             &sets[UBOS::TEXTURE_UBO]);
  }
}

void vulkan::ubos::init(U32 max_textures) {
  VkDescriptorPoolSize pool_sizes[2];
  pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = 2; // GlobalUBO + ModelUBO
  pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = max_textures;

  VkDescriptorPoolCreateInfo pool_create_info{};
  pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = array::size(pool_sizes);
  pool_create_info.pPoolSizes    = pool_sizes;
  pool_create_info.maxSets       = 3; // GlobalUBO + ModelUBO + 1 Bindless Texture Set

  ASSERT_SUCCESS(
      "failed to create descriptor pool!",
      vkCreateDescriptorPool(vulkan::_ctx.logical_device, &pool_create_info, nullptr, &pool));

  init_uniform_buffer(UBOS::GLOBAL_UBO, sizeof(GlobalUBO));
  init_uniform_buffer(UBOS::MODEL_UBO, sizeof(ModelUBO));
  init_textures_ubo(max_textures);
}

void vulkan::ubos::cleanup() {
  vulkan::buffers::cleanup(::buffers[UBOS::GLOBAL_UBO]);
  vulkan::buffers::cleanup(::buffers[UBOS::MODEL_UBO]);

  for (U32 i = 0; i < UBOS::COUNT; ++i) {
    vkDestroyDescriptorSetLayout(vulkan::_ctx.logical_device, ::layouts[i], nullptr);
  }
  vkDestroyDescriptorPool(vulkan::_ctx.logical_device, pool, nullptr);
}

void vulkan::ubos::set_global_ubo(GlobalUBO data) {
  memcpy(memory_ptrs[UBOS::GLOBAL_UBO], &data, sizeof(GlobalUBO));
}

void vulkan::ubos::set_model_ubo(ModelUBO data) {
  memcpy(memory_ptrs[UBOS::MODEL_UBO], &data, sizeof(ModelUBO));
}

void vulkan::ubos::set_textures(DynamicArray<vulkan::TextureHandle> textures) {
  printf("textures.size: %d\n", textures._size);
  VkDescriptorImageInfo image_infos[textures._size];

  for (U32 i = 0; i < textures._size; ++i) {
    image_infos[i].imageView   = *images::view(textures._data[i]);
    image_infos[i].sampler     = *texture_samplers::sampler(textures::sampler(textures._data[i]));
    image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  VkWriteDescriptorSet texture_write{};
  texture_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  texture_write.dstSet          = sets[UBOS::TEXTURE_UBO];
  texture_write.dstBinding      = 0;
  texture_write.descriptorCount = textures._size;
  texture_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture_write.pImageInfo      = image_infos;

  vkUpdateDescriptorSets(vulkan::_ctx.logical_device, 1, &texture_write, 0, nullptr);
}

void vulkan::ubos::bind_global_ubo(CommandBufferHandle cmds, PipelineHandle pipeline) {
  vkCmdBindDescriptorSets(*command_buffers::buffer(cmds),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipelines::layout(pipeline),
                          0,
                          1,
                          &sets[UBOS::GLOBAL_UBO],
                          0,
                          nullptr);
}

void vulkan::ubos::bind_model_ubo(CommandBufferHandle cmds, PipelineHandle pipeline) {
  vkCmdBindDescriptorSets(*command_buffers::buffer(cmds),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipelines::layout(pipeline),
                          1,
                          1,
                          &sets[UBOS::MODEL_UBO],
                          0,
                          nullptr);
}

void vulkan::ubos::bind_textures(CommandBufferHandle cmds, PipelineHandle pipeline) {
  vkCmdBindDescriptorSets(*command_buffers::buffer(cmds),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          *pipelines::layout(pipeline),
                          2,
                          1,
                          &sets[UBOS::TEXTURE_UBO],
                          0,
                          nullptr);
}

VkDescriptorSetLayout vulkan::ubos::global_ubo_layout() { return layouts[UBOS::GLOBAL_UBO]; }

VkDescriptorSetLayout vulkan::ubos::model_ubo_layout() { return layouts[UBOS::MODEL_UBO]; }

VkDescriptorSetLayout vulkan::ubos::textures_ubo_layout() { return layouts[UBOS::TEXTURE_UBO]; }

/* void vulkan::ubos::push_constants(CommandBufferHandle cmds, */
/*                                   PipelineHandle      pipeline, */
/*                                   PushConstants       constants) { */
/*   vkCmdPushConstants(*command_buffers::buffer(cmds), */
/*                      *pipelines::layout(pipeline), */
/*                      VK_SHADER_STAGE_FRAGMENT_BIT, */
/*                      0, */
/*                      sizeof(PushConstants), */
/*                      &constants); */
/* } */
