#include "ui.h"

#include "arena.h"

#include <SDL3/SDL_error.h>
#include <cstdio>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "renderer_SDL3.cpp"

namespace {
  const U32             FONT_ID = 0;
  ArenaHandle           mem_ui  = arena::by_name("render_ui");
  Clay_SDL3RendererData renderer_data;

  static inline Clay_Dimensions
  sdl_measure_text_cb(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    TTF_Font** fonts = static_cast<TTF_Font**>(userData);
    TTF_Font*  font  = fonts[config->fontId];
    int        width, height;

    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return Clay_Dimensions{(float)width, (float)height};
  }

  void handle_clay_errors_cb(Clay_ErrorData errorData) {
    printf("clay error: %s", errorData.errorText.chars);
  }
}

void ui::init(SDL_Window* window) {
  if (!TTF_Init()) {
    printf("failed to init SDL_TTF");
    exit(0);
  }

  renderer_data.renderer = SDL_CreateRenderer(window, "tengine");
  if (renderer_data.renderer == nullptr) {
    printf("failed to create SDL renderer for UI, %s", SDL_GetError());
    exit(0);
  }

  renderer_data.textEngine = TTF_CreateRendererTextEngine(renderer_data.renderer);
  if (!renderer_data.textEngine) {
    printf("failed to create text engine from renderer, %s", SDL_GetError());
    exit(0);
  }

  renderer_data.fonts = arena::alloc<TTF_Font*>(mem_ui, sizeof(TTF_Font*));
  if (!renderer_data.fonts) {
    printf("failed to alloc fonts memory");
    exit(0);
  }

  TTF_Font* font = TTF_OpenFont("Roboto-Regular.ttf", 24);
  if (!font) {
    printf("failed to alloc fonts memory");
    exit(0);
  }

  renderer_data.fonts[FONT_ID] = font;

  U64        mem_size   = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_Arena{
      .capacity = mem_size,
      .memory   = arena::alloc<char>(mem_ui, mem_size),
  };

  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  Clay_Initialize(clayMemory,
                  Clay_Dimensions{static_cast<F32>(width), static_cast<F32>(height)},
                  Clay_ErrorHandler{handle_clay_errors_cb});

  Clay_SetMeasureTextFunction(sdl_measure_text_cb, renderer_data.fonts);
}

void ui::update(F32 dt) {

}

void ui::cleanup() {
  
}


