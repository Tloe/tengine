#pragma once

#include "handles.h"

#include <stb/stb_truetype.h>

namespace render {
  struct Font {
    U8*                  rgba_bitmap;
    U32                  bitmap_width;
    U32                  bitmap_height;
    F32                  pixel_height;
    U32                  first_codepoint;
    U32                  codepoint_count;
    stbtt_packedchar*    packed_chars;
    TextureHandle        texture;
    TextureSamplerHandle sampler;
  };
  namespace fonts {
    FontHandle load(const char* fpath);

    void cleanup(FontHandle handle);

    Font font(FontHandle handle);
  }
}
