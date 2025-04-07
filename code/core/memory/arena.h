#pragma once

#include "handles.h"
#include "types.h"

#include <cstddef>

#define INIT_ARENA(NAME, SIZE)                                                                     \
  U8          memory_##NAME##_##__LINE__[SIZE];                                                    \
  ArenaHandle a_##NAME##_##__LINE__ = arena::set(#NAME, memory_##NAME##_##__LINE__, SIZE);

/* #define INIT_ARENA(NAME, SIZE) \ */
/*   ({ \ */
/*     U8          memory[SIZE]; \ */
/*     ArenaHandle a = arena::set(#NAME, memory, SIZE); \ */
/*     a; \ */
/*   }) */

#define SET_SCRATCH(SIZE)                                                                          \
  ({                                                                                               \
    U8 memory[SIZE];                                                                               \
    arena::set_scratch(memory, SIZE);                                                              \
  })

struct Arena {
  U8* buf;
  U32 buf_len;
  U32 prev_offset;
  U32 curr_offset;
};

namespace arena {
  const U8 DEFAULT_ALIGNMENT = 2 * sizeof(U8*);

  ArenaHandle set(const char* name, U8* mem, U32 mem_size);
  void        set_scratch(U8* mem, U32 size);

  ArenaHandle        by_name(const char* name);
  inline ArenaHandle scratch() { return ArenaHandle{.value = 0}; };

  ArenaHandle frame();
  void        next_frame();

  U8*  alloc(ArenaHandle handle, U32 size, U8 align = DEFAULT_ALIGNMENT);
  U8*  resize(ArenaHandle handle,
              U8*         old_memory,
              U32         old_size,
              U32         new_size,
              U8          align = DEFAULT_ALIGNMENT);
  void reset(ArenaHandle handle);

  template <typename T>
  T* alloc(ArenaHandle handle, U32 size, U8 align = DEFAULT_ALIGNMENT) {
    return reinterpret_cast<T*>(alloc(handle, size, align));
  }

  template <typename T>
  T resize(ArenaHandle handle,
           U8*         old_memory,
           U32         old_size,
           U32         new_size,
           U8          align = DEFAULT_ALIGNMENT) {
    return reinterpret_cast<T>(resize(handle, old_memory, old_size, new_size, align));
  }
}
