
#include "exec/fightspace/ui.h"
#include "types.h"

Clay_RenderCommandArray fightspace_ui() {
  Clay_BeginLayout();

  CLAY({
      .layout =
          {
              .sizing          = {.width  = CLAY_SIZING_FIXED(200),
                                  .height = CLAY_SIZING_FIXED(500)},
              .padding         = CLAY_PADDING_ALL(16),
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
          },
  }) {
    CLAY({
        .layout =
            {
                .sizing =
                    {
                        .width  = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                .padding         = CLAY_PADDING_ALL(16),
                .childGap        = 10,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        .backgroundColor = {.r = 107, .g = 167, .b = 149, .a = 255},
        .border =
            {
                .color = {.r = 255, .g = 255, .b = 255, .a = 255},
                .width = {2, 2, 2, 2, 0},
            },
    }) {
      CLAY({
          .layout          = {.sizing = {.width  = CLAY_SIZING_PERCENT(.9),
                                         .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {.r = 0, .g = 0, .b = 100, .a = 255},
          .cornerRadius    = CLAY_CORNER_RADIUS(8),
          .border =
              {
                  .color = {.r = 255, .g = 189, .b = 39, .a = 255},
                  .width = {2, 2, 2, 2, 0},
              },
      }) {
        CLAY({.layout = {.sizing         = {.width = CLAY_SIZING_GROW(0)},
                         .padding        = CLAY_PADDING_ALL(16),
                         .childGap       = 16,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         }}}) {
          CLAY_TEXT(CLAY_STRING("Howdy"),
                    CLAY_TEXT_CONFIG({
                        .textColor = {255, 255, 255, 255},
                        .fontId    = 0,
                        .fontSize  = 16,
                    }));
        }
      }
      CLAY({
          .layout          = {.sizing = {.width  = CLAY_SIZING_PERCENT(0.9),
                                         .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {.r = 100, .g = 0, .b = 0, .a = 255},
          .cornerRadius    = CLAY_CORNER_RADIUS(8),
          .border =
              {
                  .color = {.r = 255, .g = 189, .b = 39, .a = 255},
                  .width = {2, 2, 2, 2, 0},
              },
      }) {
        // etc
      }
    }
  }

  Clay_RenderCommandArray clay_commands = Clay_EndLayout();
  for (I32 i = 0; i < clay_commands.length; i++) {
    Clay_RenderCommandArray_Get(&clay_commands, i)->boundingBox.y += 0;
  }

  return clay_commands;
}
