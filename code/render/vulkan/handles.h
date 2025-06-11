#pragma once

#include "handle.h"
#include "types.h"

namespace vulkan {
  struct CommandBufferTag;
  typedef Handle<CommandBufferTag, U16, U16_MAX> CommandBufferHandle;

  struct BufferTag;
  typedef Handle<BufferTag, U16, U16_MAX> BufferHandle;
  typedef BufferHandle                    VertexBufferHandle;
  typedef BufferHandle                    IndexBufferHandle;
  typedef BufferHandle                    UBOBufferHandle;

  struct UBOTag;
  typedef Handle<UBOTag, U16, U16_MAX> UBOHandle;

  struct ImageTag;
  typedef Handle<ImageTag, U16, U16_MAX> ImageHandle;

  typedef ImageHandle TextureHandle;

  struct TextureSamplerTag;
  typedef Handle<TextureSamplerTag, U16, U16_MAX> TextureSamplerHandle;

  struct PipelineTag;
  typedef Handle<PipelineTag, U64, U64_MAX> PipelineHandle;

  struct RenderPassTag;
  typedef Handle<RenderPassTag, U16, U16_MAX> RenderPassHandle;

  struct DescriptorSetTag;
  typedef Handle<DescriptorSetTag, U16, U16_MAX> DescriptorSetHandle;
}
