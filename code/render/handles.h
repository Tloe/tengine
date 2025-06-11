#pragma once

#include "handle.h"
#include "types.h"
#include "vulkan/handles.h"

struct MeshTag;
typedef Handle<MeshTag, U16, U16_MAX> MeshHandle;

typedef vulkan::TextureHandle TextureHandle;

typedef vulkan::TextureSamplerHandle TextureSamplerHandle;

struct FontTag;
typedef Handle<FontTag, U16, U16_MAX> FontHandle;
