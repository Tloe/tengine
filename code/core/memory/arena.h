#ifndef ARENA_H
#define ARENA_H

#include "types.h"
#include <cstddef>

#define INIT_ARENA(SIZE)                                                       \
  ({                                                                           \
    U8         memory[SIZE];                                                   \
    mem::Arena a = mem::arena::init(memory, SIZE);                             \
    a;                                                                         \
  })

namespace mem {
  struct Arena {
    U8* buf;
    U32 buf_len;
    U32 prev_offset;
    U32 curr_offset;
  };

  typedef Arena* ArenaPtr;

  namespace arena {
    const U8 DEFAULT_ALIGNMENT = 2 * sizeof(U8*);

    Arena init(U8* mem, U32 mem_size);

    U8*  alloc(ArenaPtr a, U32 size, U8 align = DEFAULT_ALIGNMENT);
    U8*  resize(ArenaPtr a,
                U8*      old_memory,
                U32      old_size,
                U32      new_size,
                U8       align = DEFAULT_ALIGNMENT);
    void reset(ArenaPtr a);

    template <typename T>
    T* alloc(ArenaPtr a, U32 size, U8 align = DEFAULT_ALIGNMENT) {
      return reinterpret_cast<T*>(alloc(a, size, align));
    }

    template <typename T>
    T resize(ArenaPtr a,
             U8*      old_memory,
             U32      old_size,
             U32      new_size,
             U8       align = DEFAULT_ALIGNMENT) {
      return reinterpret_cast<T>(
          resize(a, old_memory, old_size, new_size, align));
    }
  }
}

#endif
