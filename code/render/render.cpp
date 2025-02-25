#include "render.h"

#include "arena.h"
#include "vulkan/context.h"
#include "vulkan/device.h"
#include "vulkan/image.h"
#include "vulkan/swap_chain.h"
#include "vulkan/vulkan.h"
#include "vulkan/window.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "vertex_buffer.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace {
#ifdef NDEBUG
  const bool debug = false;
#else
  const bool debug = true;
#endif
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout      pipelineLayout;

  VkPipeline                   graphicsPipeline;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore>     imageAvailableSemaphores;
  std::vector<VkSemaphore>     renderFinishedSemaphores;
  std::vector<VkFence>         inFlightFences;
  U32                          currentFrame       = 0;
  bool                         framebufferResized = false;

  VkBuffer       vertexBuffer;
  VkDeviceMemory vertexBufferMemory;

  VkBuffer       indexBuffer;
  VkDeviceMemory indexBufferMemory;

  std::vector<VkBuffer>       uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*>          uniformBuffersMapped;

  struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

  std::vector<render::Vertex> vertices;
  std::vector<uint32_t>       indices;

  VkDescriptorPool             descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  uint32_t            mipLevels;
  vulkan::ImageHandle textureImage;
  VkSampler           textureSampler;

  const uint32_t WIDTH  = 1200;
  const uint32_t HEIGHT = 1024;

  const std::string MODEL_PATH   = "viking_room.obj";
  const std::string TEXTURE_PATH = "viking_room.png";

  const int MAX_FRAMES_IN_FLIGHT = 2;

  const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  static std::vector<char> readFile(const char* filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      printf("failed to open file!");
      exit(0);
    }

    size_t            fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }

  VkShaderModule
  createShaderModule(vulkan::ContextPtr ctx, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(ctx->logical_device,
                             &createInfo,
                             nullptr,
                             &shaderModule) != VK_SUCCESS) {
      printf("failed to create shader module!");
      exit(0);
    }

    return shaderModule;
  }

  void createGraphicsPipeline(vulkan::ContextPtr   ctx,
                              vulkan::SwapChainPtr swap_chain,
                              VkRenderPass         render_pass,
                              mem::Arena*          scratch) {
    // programming stages
    auto vertShader = readFile("vert.spv");
    auto fragShader = readFile("frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(ctx, vertShader);
    VkShaderModule fragShaderModule = createShaderModule(ctx, fragShader);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    auto bindingDescription = render::vertex::getBindingDescription();
    auto attributeDescriptions =
        render::vertex::getAttributeDescriptions(scratch);

    // fixed function stages
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        attributeDescriptions.size;
    vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swap_chain->extent.width);
    viewport.height   = static_cast<float>(swap_chain->extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynamicStates;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain->extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp          = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = ctx->max_msaa_samples;
    multisampling.minSampleShading      = 1.0f;     // Optional
    multisampling.pSampleMask           = nullptr;  // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable      = VK_FALSE; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds        = 0.0f; // Optional
    depthStencil.maxDepthBounds        = 1.0f; // Optional
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {}; // Optional
    depthStencil.back                  = {}; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;                    // Optional
    pipelineLayoutInfo.pSetLayouts    = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;            // Optional
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;      // Optional

    if (vkCreatePipelineLayout(ctx->logical_device,
                               &pipelineLayoutInfo,
                               nullptr,
                               &pipelineLayout) != VK_SUCCESS) {

      printf("failed to create pipeline layout!");
      exit(0);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;

    pipelineInfo.layout     = pipelineLayout;
    pipelineInfo.renderPass = render_pass;

    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex  = -1;             // Optional

    if (vkCreateGraphicsPipelines(ctx->logical_device,
                                  VK_NULL_HANDLE,
                                  1,
                                  &pipelineInfo,
                                  nullptr,
                                  &graphicsPipeline) != VK_SUCCESS) {
      printf("failed to create graphics pipeline!");
      exit(0);
    }

    vkDestroyShaderModule(ctx->logical_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(ctx->logical_device, vertShaderModule, nullptr);
  }

  VkRenderPass create_render_pass(vulkan::ContextPtr ctx) {
    auto surface_format = vulkan::context::surface_format(ctx);

    VkAttachmentDescription color_attachment{};
    color_attachment.format         = surface_format.format;
    color_attachment.samples        = ctx->max_msaa_samples;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.format         = surface_format.format;
    color_attachment_resolve.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_resolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_resolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_resolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_resolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format =
        vulkan::device::find_depth_format(ctx->physical_device);
    depth_attachment.samples        = ctx->max_msaa_samples;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_resolve_ref{};
    color_attachment_resolve_ref.attachment = 2;
    color_attachment_resolve_ref.layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    subpass.pResolveAttachments     = &color_attachment_resolve_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass   = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color_attachment,
                                             depth_attachment,
                                             color_attachment_resolve};

    VkRenderPassCreateInfo create_info{};
    create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = ds::array::size(attachments);
    create_info.pAttachments    = attachments;
    create_info.subpassCount    = 1;
    create_info.pSubpasses      = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies   = &dependency;

    VkRenderPass render_pass;
    if (vkCreateRenderPass(ctx->logical_device,
                           &create_info,
                           nullptr,
                           &render_pass) != VK_SUCCESS) {
      printf("failed to create render pass!");
      exit(0);
    }

    return render_pass;
  }

  void recordCommandBuffer(vulkan::SwapChainPtr swap_chain,
                           VkRenderPass         render_pass,
                           VkCommandBuffer      command_buffer,
                           U32                  image_index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
      printf("failed to begin recording command buffer!");
      exit(0);
    }

    VkClearValue clear_values[2];
    clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass  = render_pass;
    render_pass_info.framebuffer = swap_chain->frame_buffers._data[image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swap_chain->extent;
    render_pass_info.clearValueCount   = ds::array::size(clear_values);
    render_pass_info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(command_buffer,
                         &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<F32>(swap_chain->extent.width);
    viewport.height   = static_cast<F32>(swap_chain->extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain->extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer     vertex_buffers[] = {vertexBuffer};
    VkDeviceSize offsets[]        = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1,
                            &descriptorSets[currentFrame],
                            0,
                            nullptr);
    vkCmdDrawIndexed(command_buffer,
                     static_cast<U32>(indices.size()),
                     1,
                     0,
                     0,
                     0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
      printf("failed to record command buffer!");
      exit(0);
    }
  }

  void createCommandBuffers(vulkan::Context& ctx) {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.command_pool;
    allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.size();

    if (vkAllocateCommandBuffers(ctx.logical_device,
                                 &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS) {
      printf("failed to allocate command buffers!");
      exit(0);
    }
  }

  void createSyncObjects(vulkan::Context& ctx) {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(ctx.logical_device,
                            &semaphoreInfo,
                            nullptr,
                            &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(ctx.logical_device,
                            &semaphoreInfo,
                            nullptr,
                            &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(ctx.logical_device,
                        &fenceInfo,
                        nullptr,
                        &inFlightFences[i]) != VK_SUCCESS) {
        printf("failed to create semaphores!");
        exit(0);
      }
    }
  }

  void updateUniformBuffer(vulkan::SwapChainPtr swap_chain, U32 currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f),
                            time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj =
        glm::perspective(glm::radians(45.0f),
                         swap_chain->extent.width /
                             static_cast<float>(swap_chain->extent.height),
                         0.1f,
                         10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

  void recreateSwapChain(vulkan::ContextPtr   ctx,
                         vulkan::SwapChainPtr swap_chain,
                         VkRenderPass         render_pass,
                         mem::ArenaPtr        frame,
                         SDL_Window*          window) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    while (width == 0 || height == 0) {
      SDL_GetWindowSize(window, &width, &height);
      SDL_Event e;
      SDL_PollEvent(&e);
    }

    vkDeviceWaitIdle(ctx->logical_device);

    vulkan::swap_chain::cleanup(ctx, swap_chain);

    *swap_chain = vulkan::swap_chain::create(ctx, frame, window);

    vulkan::swap_chain::create_framebuffers(ctx, render_pass, swap_chain);
  }

  void drawFrame(vulkan::ContextPtr   ctx,
                 vulkan::SwapChainPtr swap_chain,
                 VkRenderPass         render_pass,
                 mem::ArenaPtr        frame,
                 SDL_Window*          window) {

    vkWaitForFences(ctx->logical_device,
                    1,
                    &inFlightFences[currentFrame],
                    VK_TRUE,
                    UINT64_MAX);

    U32      image_index;
    VkResult result =
        vkAcquireNextImageKHR(ctx->logical_device,
                              swap_chain->swap_chain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE,
                              &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain(ctx, swap_chain, render_pass, frame, window);
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      printf("failed to acquire swap chain image!");
      exit(0);
    }

    updateUniformBuffer(swap_chain, currentFrame);

    vkResetFences(ctx->logical_device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(swap_chain,
                        render_pass,
                        commandBuffers[currentFrame],
                        image_index);

    VkSemaphore wait_semaphores[]   = {imageAvailableSemaphores[currentFrame]};
    VkSemaphore signal_semaphores[] = {renderFinishedSemaphores[currentFrame]};
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &commandBuffers[currentFrame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    if (vkQueueSubmit(ctx->graphics_queue,
                      1,
                      &submit_info,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
      printf("failed to submit draw command buffer!");
      exit(0);
    }

    VkSwapchainKHR swap_chains[] = {swap_chain->swap_chain};

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = swap_chains;
    present_info.pImageIndices      = &image_index;
    present_info.pResults           = nullptr; // Optional

    result = vkQueuePresentKHR(ctx->present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
      framebufferResized = false;
      recreateSwapChain(ctx, swap_chain, render_pass, frame, window);
    } else if (result != VK_SUCCESS) {
      printf("failed to present swap chain image!");
      exit(0);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void createBuffer(vulkan::ContextPtr    ctx,
                    VkDeviceSize          size,
                    VkBufferUsageFlags    usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer&             buffer,
                    VkDeviceMemory&       bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx->logical_device, &bufferInfo, nullptr, &buffer) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx->logical_device,
                                  buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        vulkan::device::memory_type(ctx->physical_device,
                                    memRequirements.memoryTypeBits,
                                    properties);

    if (vkAllocateMemory(ctx->logical_device,
                         &allocInfo,
                         nullptr,
                         &bufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(ctx->logical_device, buffer, bufferMemory, 0);
  }

  void copyBuffer(vulkan::ContextPtr ctx,
                  VkBuffer           srcBuffer,
                  VkBuffer           dstBuffer,
                  VkDeviceSize       size) {
    VkCommandBuffer command_buffer = vulkan::begin_single_time_commands(ctx);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(command_buffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vulkan::end_single_time_commands(ctx, command_buffer);
  }

  void createVertexBuffer(vulkan::ContextPtr ctx) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(ctx,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void* data;
    vkMapMemory(ctx->logical_device,
                stagingBufferMemory,
                0,
                bufferSize,
                0,
                &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx->logical_device, stagingBufferMemory);

    createBuffer(ctx,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vertexBuffer,
                 vertexBufferMemory);

    copyBuffer(ctx, stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(ctx->logical_device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->logical_device, stagingBufferMemory, nullptr);
  }

  void createIndexBuffer(vulkan::ContextPtr ctx) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(ctx,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void* data;
    vkMapMemory(ctx->logical_device,
                stagingBufferMemory,
                0,
                bufferSize,
                0,
                &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(ctx->logical_device, stagingBufferMemory);

    createBuffer(ctx,
                 bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 indexBuffer,
                 indexBufferMemory);

    copyBuffer(ctx, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(ctx->logical_device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->logical_device, stagingBufferMemory, nullptr);
  }

  void createDescriptorSetLayout(vulkan::ContextPtr ctx) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding            = 0;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding         = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(ctx->logical_device,
                                    &layoutInfo,
                                    nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
      printf("failed to create descriptor set layout!");
      exit(0);
    }
  }

  void createUniformBuffers(vulkan::ContextPtr ctx) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      createBuffer(ctx,
                   bufferSize,
                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   uniformBuffers[i],
                   uniformBuffersMemory[i]);

      vkMapMemory(ctx->logical_device,
                  uniformBuffersMemory[i],
                  0,
                  bufferSize,
                  0,
                  &uniformBuffersMapped[i]);
    }
  }

  void createDescriptorPool(vulkan::ContextPtr ctx) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(ctx->logical_device,
                               &poolInfo,
                               nullptr,
                               &descriptorPool) != VK_SUCCESS) {
      printf("failed to create descriptor pool!");
      exit(0);
    }
  }

  void createDescriptorSets(vulkan::ContextPtr  ctx,
                            vulkan::ImageHandle texture_image) {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               descriptorSetLayout);
    VkDescriptorSetAllocateInfo        allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts        = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(ctx->logical_device,
                                 &allocInfo,
                                 descriptorSets.data()) != VK_SUCCESS) {
      printf("failed to allocate descriptor sets!");
      exit(0);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VkDescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = uniformBuffers[i];
      bufferInfo.offset = 0;
      bufferInfo.range  = sizeof(UniformBufferObject);

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView   = *vulkan::image::view(texture_image);
      imageInfo.sampler     = textureSampler;

      std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

      descriptorWrites[0].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet     = descriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo     = &bufferInfo;

      descriptorWrites[1].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet     = descriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pImageInfo      = &imageInfo;

      vkUpdateDescriptorSets(ctx->logical_device,
                             static_cast<U32>(descriptorWrites.size()),
                             descriptorWrites.data(),
                             0,
                             nullptr);
    }
  }

  void copyBufferToImage(vulkan::ContextPtr ctx,
                         VkBuffer           buffer,
                         VkImage            image,
                         uint32_t           width,
                         uint32_t           height) {
    VkCommandBuffer command_buffer = vulkan::begin_single_time_commands(ctx);

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer,
                           buffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);

    vulkan::end_single_time_commands(ctx, command_buffer);
  }

  void createTextureImage(vulkan::ContextPtr ctx) {
    int          texWidth, texHeight, texChannels;
    stbi_uc*     pixels    = stbi_load(TEXTURE_PATH.c_str(),
                                &texWidth,
                                &texHeight,
                                &texChannels,
                                STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    mipLevels = static_cast<uint32_t>(
                    std::floor(std::log2(std::max(texWidth, texHeight)))) +
                1;

    if (!pixels) {
      printf("failed to load texture image!");
      exit(0);
    }

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(ctx,
                 imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer,
                 stagingBufferMemory);

    void* data;
    vkMapMemory(ctx->logical_device,
                stagingBufferMemory,
                0,
                imageSize,
                0,
                &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(ctx->logical_device, stagingBufferMemory);

    stbi_image_free(pixels);

    textureImage = vulkan::image::create(texWidth,
                                         texHeight,
                                         VK_FORMAT_R8G8B8A8_SRGB,
                                         VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                         mipLevels,
                                         VK_SAMPLE_COUNT_1_BIT);

    vulkan::image::transition_layout(textureImage,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(ctx,
                      stagingBuffer,
                      *vulkan::image::image(textureImage),
                      texWidth,
                      texHeight);

    vkDestroyBuffer(ctx->logical_device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->logical_device, stagingBufferMemory, nullptr);

    vulkan::image::create_view(textureImage, VK_IMAGE_ASPECT_COLOR_BIT);

    vulkan::image::generate_mipmaps(textureImage, texWidth, texHeight);
  }

  void createTextureSampler(vulkan::Context& ctx) {
    auto properties = vulkan::device::properties(ctx.physical_device);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter        = VK_FILTER_LINEAR;
    samplerInfo.minFilter        = VK_FILTER_LINEAR;
    samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy    = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    // see end of this for these settings
    // https://vulkan-tutorial.com/en/Generating_Mipmaps
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod     = 0.0f; // Optional
    samplerInfo.maxLod     = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f; // Optional

    if (vkCreateSampler(ctx.logical_device,
                        &samplerInfo,
                        nullptr,
                        &textureSampler) != VK_SUCCESS) {
      printf("failed to create texture sampler!");
      exit(0);
    }
  }

  void loadModel() {
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn, err;

    if (!tinyobj::LoadObj(&attrib,
                          &shapes,
                          &materials,
                          &warn,
                          &err,
                          MODEL_PATH.c_str())) {
      throw std::runtime_error(warn + err);
    }

    std::unordered_map<render::Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        render::Vertex vertex{};

        vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                      attrib.vertices[3 * index.vertex_index + 1],
                      attrib.vertices[3 * index.vertex_index + 2]};

        vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                           1.0f -
                               attrib.texcoords[2 * index.texcoord_index + 1]};

        vertex.color = {1.0f, 1.0f, 1.0f};

        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
          vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
      }
    }
  }
}

render::Renderer render::init(mem::ArenaPtr a) {

  Renderer renderer;
  renderer.window = vulkan::window::init(WIDTH, HEIGHT);

  renderer.ctx = vulkan::context::create(a, renderer.window, debug);

  vulkan::image::init(&renderer.ctx, a);

  renderer.swap_chain =
      vulkan::swap_chain::create(&renderer.ctx, a, renderer.window);

  renderer.render_pass = create_render_pass(&renderer.ctx);

  vulkan::swap_chain::create_framebuffers(&renderer.ctx,
                                          renderer.render_pass,
                                          &renderer.swap_chain);

  createDescriptorSetLayout(&renderer.ctx);
  createGraphicsPipeline(&renderer.ctx,
                         &renderer.swap_chain,
                         renderer.render_pass,
                         a);

  createTextureImage(&renderer.ctx);
  createTextureSampler(renderer.ctx);

  loadModel();
  createVertexBuffer(&renderer.ctx);
  createIndexBuffer(&renderer.ctx);
  createUniformBuffers(&renderer.ctx);
  createDescriptorPool(&renderer.ctx);
  createDescriptorSets(&renderer.ctx, textureImage);

  createCommandBuffers(renderer.ctx);
  createSyncObjects(renderer.ctx);

  return renderer;
}

void render::update(Renderer& renderer, mem::ArenaPtr a) {
  bool quit = false;

  while (!quit) {
    SDL_Event e;
    SDL_zero(e);
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_EVENT_QUIT: {
          quit = true;
          break;
        }
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
          framebufferResized = true;
        }
      }
    }

    drawFrame(&renderer.ctx,
              &renderer.swap_chain,
              renderer.render_pass,
              a,
              renderer.window);
  }

  vkDeviceWaitIdle(renderer.ctx.logical_device);
}

void render::cleanup(render::Renderer& renderer) {
  vulkan::swap_chain::cleanup(&renderer.ctx, &renderer.swap_chain);

  vkDestroySampler(renderer.ctx.logical_device, textureSampler, nullptr);
  vulkan::image::cleanup(textureImage);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(renderer.ctx.logical_device, uniformBuffers[i], nullptr);
    vkFreeMemory(renderer.ctx.logical_device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(renderer.ctx.logical_device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(renderer.ctx.logical_device,
                               descriptorSetLayout,
                               nullptr);

  vkDestroyBuffer(renderer.ctx.logical_device, indexBuffer, nullptr);
  vkFreeMemory(renderer.ctx.logical_device, indexBufferMemory, nullptr);

  vkDestroyBuffer(renderer.ctx.logical_device, vertexBuffer, nullptr);
  vkFreeMemory(renderer.ctx.logical_device, vertexBufferMemory, nullptr);

  vkDestroyPipeline(renderer.ctx.logical_device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(renderer.ctx.logical_device, pipelineLayout, nullptr);

  vkDestroyRenderPass(renderer.ctx.logical_device,
                      renderer.render_pass,
                      nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(renderer.ctx.logical_device,
                       renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(renderer.ctx.logical_device,
                       imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(renderer.ctx.logical_device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(renderer.ctx.logical_device,
                       renderer.ctx.command_pool,
                       nullptr);

  vulkan::context::cleanup(renderer.ctx, debug);

  vulkan::window::cleanup(renderer.window);

  SDL_Quit();
}
