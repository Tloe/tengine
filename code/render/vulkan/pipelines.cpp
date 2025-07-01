#include "pipelines.h"

#include "context.h"
#include "ds_array_dynamic.h"
#include "ds_hashmap.h"
#include "handle.h"
#include "handles.h"
#include "io.h"
#include "meshes.h"
#include "render_pass.h"
#include "swap_chain.h"
#include "common.h"
#include "vulkan/command_buffers.h"
#include "vulkan/ubos.h"

#include <cstdio>
#include "vulkan_include.h"

namespace {
  ArenaHandle mem_render = arena::by_name("render");

  handles::Allocator<vulkan::PipelineHandle, U8, 16> _handles;

  struct PipelineData {
    VkPipeline                      pipeline;
    VkPipelineLayout                layout;
    DynamicArray<vulkan::UBOHandle> ubos;
  };

  auto _pipelines = hashmap::init64<PipelineData>(mem_render);

  VkShaderModule create_shader_module(const char* fpath) {
    auto code = read_file(arena::scratch(), fpath);

    VkShaderModuleCreateInfo create_info{};
    create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code._size;
    create_info.pCode    = reinterpret_cast<const U32*>(code._data);

    VkShaderModule shader_module;

    ASSERT_SUCCESS(
        "failed to create shader module!",
        vkCreateShaderModule(vulkan::_ctx.logical_device, &create_info, nullptr, &shader_module));

    return shader_module;
  }
}

vulkan::PipelineHandle vulkan::pipelines::create(RenderPassHandle render_pass, Settings settings) {
  // programming stages
  auto vertex_shader        = create_shader_module(settings.vertex_shader_fpath);
  auto fragmentation_shader = create_shader_module(settings.fragment_shader_fpath);

  VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
  vertex_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_shader_stage_info.module = vertex_shader;
  vertex_shader_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
  fragment_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_shader_stage_info.module = fragmentation_shader;
  fragment_shader_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {
      vertex_shader_stage_info,
      fragment_shader_stage_info,
  };

  // fixed function stages
  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount   = 1;
  vertex_input_info.pVertexBindingDescriptions      = &settings.binding_description;
  vertex_input_info.vertexAttributeDescriptionCount = settings.attribute_descriptions_format_count;
  vertex_input_info.pVertexAttributeDescriptions    = settings.attribute_descriptions;

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<F32>(vulkan::_swap_chain.extent.width);
  viewport.height   = static_cast<F32>(vulkan::_swap_chain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates    = dynamic_states;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = vulkan::_swap_chain.extent;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports    = &viewport;
  viewport_state.scissorCount  = 1;
  viewport_state.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  // if(settings.disable_depth_testing) {
  //   rasterizer.cullMode                = VK_CULL_MODE_NONE;
  // } else {
  //   rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  // }
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp          = 0.0f;
  rasterizer.depthBiasSlopeFactor    = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = vulkan::_ctx.max_msaa_samples;
  multisampling.minSampleShading      = 1.0f;
  multisampling.pSampleMask           = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable      = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depth_stencil{};
  depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.minDepthBounds        = 0.0f;
  depth_stencil.maxDepthBounds        = 1.0f;
  depth_stencil.stencilTestEnable     = VK_FALSE;
  depth_stencil.front                 = {};
  depth_stencil.back                  = {};

  if (settings.disable_depth_testing) {
    depth_stencil.depthTestEnable  = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_ALWAYS;
  } else {
    depth_stencil.depthTestEnable  = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
  }

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  if (settings.disable_depth_testing) {
    printf("enable blending!!!!!!\n"); 
    // FIXME this is just a hack to set this for ui
    color_blend_attachment.blendEnable         = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD,
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  } else {
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable         = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  }

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  VkDescriptorSetLayout ubo_layouts[settings.ubos._size];
  for (U32 i = 0; i < settings.ubos._size; i++) {
    ubo_layouts[i] = vulkan::ubos::descriptor_set_layout(settings.ubos._data[i]);
  };

  VkPushConstantRange push_constant_range = {};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.offset     = 0;
  push_constant_range.size       = sizeof(MeshGPUConstants);

  VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
  pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.pSetLayouts    = ubo_layouts;
  pipeline_layout_create_info.setLayoutCount = settings.ubos._size;
  pipeline_layout_create_info.pushConstantRangeCount = 1;
  pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

  PipelineHandle handle = handles::next(_handles);

  auto pipeline_data = hashmap::insert(_pipelines, handle.value);

  pipeline_data->ubos = array::init(mem_render, settings.ubos);

  ASSERT_SUCCESS("failed to create pipeline layout!",
                 vkCreatePipelineLayout(vulkan::_ctx.logical_device,
                                        &pipeline_layout_create_info,
                                        nullptr,
                                        &pipeline_data->layout));

  VkGraphicsPipelineCreateInfo pipeline_create_info{};
  pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.stageCount          = 2;
  pipeline_create_info.pStages             = shader_stages;
  pipeline_create_info.pVertexInputState   = &vertex_input_info;
  pipeline_create_info.pInputAssemblyState = &input_assembly;
  pipeline_create_info.pViewportState      = &viewport_state;
  pipeline_create_info.pRasterizationState = &rasterizer;
  pipeline_create_info.pMultisampleState   = &multisampling;
  pipeline_create_info.pDepthStencilState  = &depth_stencil;
  pipeline_create_info.pColorBlendState    = &color_blending;
  pipeline_create_info.pDynamicState       = &dynamic_state;
  pipeline_create_info.layout              = pipeline_data->layout;
  pipeline_create_info.renderPass          = *vulkan::render_pass::render_pass(render_pass);
  pipeline_create_info.subpass             = 0;
  pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
  pipeline_create_info.basePipelineIndex   = -1;

  ASSERT_SUCCESS("failed to create graphics pipeline!",
                 vkCreateGraphicsPipelines(vulkan::_ctx.logical_device,
                                           VK_NULL_HANDLE,
                                           1,
                                           &pipeline_create_info,
                                           nullptr,
                                           &pipeline_data->pipeline));

  vkDestroyShaderModule(vulkan::_ctx.logical_device, fragmentation_shader, nullptr);
  vkDestroyShaderModule(vulkan::_ctx.logical_device, vertex_shader, nullptr);

  return handle;
}

void vulkan::pipelines::cleanup(PipelineHandle pipeline) {
  auto pipeline_data = hashmap::value(_pipelines, pipeline.value);

  vkDestroyPipelineLayout(vulkan::_ctx.logical_device, pipeline_data->layout, nullptr);
  vkDestroyPipeline(vulkan::_ctx.logical_device, pipeline_data->pipeline, nullptr);

  handles::free(_handles, pipeline);
}

void vulkan::pipelines::cleanup() {
  hashmap::for_each(_pipelines, [](decltype(_pipelines)::KeyValue kv) {
    vulkan::pipelines::cleanup(PipelineHandle{.value = kv.k});
  });
}

void vulkan::pipelines::bind(PipelineHandle pipeline, CommandBufferHandle command_buffer) {
  auto pipeline_data = hashmap::value(_pipelines, pipeline.value);

  vkCmdBindPipeline(*vulkan::command_buffers::buffer(command_buffer),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_data->pipeline);

  for (U32 i = 0; i < pipeline_data->ubos._size; ++i) {
    ubos::bind(pipeline_data->ubos._data[i], command_buffer, pipeline);
  }
}

VkPipeline* vulkan::pipelines::pipeline(PipelineHandle pipeline) {
  auto pipeline_data = hashmap::value(_pipelines, pipeline.value);
  return &pipeline_data->pipeline;
}

VkPipelineLayout* vulkan::pipelines::layout(PipelineHandle pipeline) {
  auto pipeline_data = hashmap::value(_pipelines, pipeline.value);
  return &pipeline_data->layout;
}
