#ifndef ARENA_H
#define ARENA_H

#include "types.h"

namespace mem {
  struct Arena {
    U8 *buf;
    U32 buf_len;
    U32 prev_offset;
    U32 curr_offset;
  };

  namespace arena {
    const U8 DEFAULT_ALIGNMENT = 2 * sizeof(void *);

    Arena init(U8 *mem, U32 mem_size);
    U8 *alloc(Arena *a, U32 size, U8 align = DEFAULT_ALIGNMENT);
    U8 *resize(Arena *a, U8 *old_memory, U32 old_size, U32 new_size, U8 align = DEFAULT_ALIGNMENT);
    void free_all(Arena &a);

    template <typename T> T* alloc(Arena *a, U32 size, U8 align = DEFAULT_ALIGNMENT) {
      return reinterpret_cast<T*>(alloc(a, size, align));
    }

    template <typename T>
    T resize(Arena *a, U8 *old_memory, U32 old_size, U32 new_size, U8 align = DEFAULT_ALIGNMENT) {
      return reinterpret_cast<T>(resize(a, old_memory, old_size, new_size, align));
    }
  }
}

#endif
