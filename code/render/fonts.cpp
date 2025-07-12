#include "fonts.h"

#include "arena.h"
#include "ds_array_static.h"
#include "handle.h"
#include "handles.h"
#include "textures.h"
#include "vulkan/textures.h"
#include "vulkan/vulkan_include.h"

#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

namespace {
  ArenaHandle mem_render = arena::by_name("render");

  auto _fonts      = array::init<render::Font, 10>(mem_render);
  U16  next_handle = 0;
}

FontHandle render::fonts::load(const char* fpath) {
  std::ifstream file_stream(fpath, std::ios::binary);

  file_stream.seekg(0, std::ios::end);
  U32 file_size = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);

  U8* ttf_buffer;
  ttf_buffer = arena::alloc(mem_render, file_size);
  file_stream.read(reinterpret_cast<char*>(ttf_buffer), file_size);

  U32 font_count = stbtt_GetNumberOfFonts(ttf_buffer);
  if (font_count == -1) {
    printf("The font file doesn't correspond to valid font data");
    exit(0);
  } else {
    printf("The File %s contains %d font(s)\n", fpath, font_count);
  }

  assert(next_handle < 10);
  auto handle = FontHandle{.value = next_handle++};

  auto font = &_fonts.data[handle.value];

  font->bitmap_height   = 1024;
  font->bitmap_width    = 1024;
  font->pixel_height    = 32.0f;
  font->first_codepoint = 32;
  font->codepoint_count = 96;
  font->rgba_bitmap =
      arena::alloc(mem_render, sizeof(U32) * font->bitmap_width * font->bitmap_height);
  memset(font->rgba_bitmap, 0, 4 * font->bitmap_width * font->bitmap_height);

  stbtt_pack_context ctx;

  U8* bitmap = arena::alloc(arena::scratch(), font->bitmap_width * font->bitmap_height);

  memset(bitmap, 0, font->bitmap_width * font->bitmap_height);
  stbtt_PackBegin(&ctx, bitmap, font->bitmap_width, font->bitmap_height, 0, 1, nullptr);

  font->packed_chars = reinterpret_cast<stbtt_packedchar*>(
      arena::alloc(mem_render, sizeof(stbtt_packedchar) * font->codepoint_count));

  stbtt_PackFontRange(&ctx,
                      ttf_buffer,
                      0,
                      font->pixel_height,
                      font->first_codepoint,
                      font->codepoint_count,
                      font->packed_chars);

  stbtt_PackEnd(&ctx);

  for (U32 i = 0; i < font->bitmap_width * font->bitmap_height; ++i) {
    U8 alpha                     = bitmap[i];
    font->rgba_bitmap[i * 4 + 0] = 255;   // R
    font->rgba_bitmap[i * 4 + 1] = 255;   // G
    font->rgba_bitmap[i * 4 + 2] = 255;   // B
    font->rgba_bitmap[i * 4 + 3] = alpha; // A
  }

  font->texture = textures::create(font->bitmap_width,
                                   font->bitmap_height,
                                   font->rgba_bitmap,
                                   sizeof(U32) * font->bitmap_width * font->bitmap_height,
                                   VK_FORMAT_R8G8B8A8_UNORM);

  font->sampler = textures::create_sampler(1);
  textures::set_sampler(font->texture, font->sampler);

  return handle;
}

void render::fonts::cleanup(FontHandle handle) {
  auto font = _fonts.data[handle.value];

  textures::cleanup(font.texture);
  textures::cleanup(font.sampler);
}

render::Font render::fonts::font(FontHandle handle) {
  return _fonts.data[handle.value];
}
