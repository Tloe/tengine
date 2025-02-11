#include "arena.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>

namespace {
  bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

  U32 align_forward(U8 *ptr, U8 align) {
    auto p = reinterpret_cast<uintptr_t>(ptr);

    assert(is_power_of_two(align));

    auto modulo = p & (align - 1);

    if (modulo != 0) {
      p += align - modulo;
    }
    return p;
  }
}

mem::Arena mem::arena::init(U8 *mem, U32 mem_size) {
  mem::Arena a;
  a.buf = mem;
  a.buf_len = mem_size;
  a.curr_offset = 0;
  a.prev_offset = 0;

  return a;
}

U8 *mem::arena::alloc(mem::Arena *a, U32 size, U8 align) {
  auto curr_ptr = a->buf + a->curr_offset;
  U32 offset = align_forward(curr_ptr, align);
  offset -= reinterpret_cast<uintptr_t>(a->buf);

  if (offset + size <= a->buf_len) {
    U8 *ptr = &a->buf[offset];
    a->prev_offset = offset;
    a->curr_offset = offset + size;

    memset(ptr, 0, size);
    return ptr;
  }

  assert(0 && "Memory is out of bounds of the buffer in this arena");
  return nullptr;
}

U8 *mem::arena::resize(Arena *a, U8 *old_mem, U32 old_size, U32 new_size, U8 align) {
  assert(is_power_of_two(align));

  if (old_mem == nullptr || old_size == 0) {
    return alloc(a, new_size, align);
  } else if (a->buf <= old_mem && old_mem < a->buf + a->buf_len) {
    if (a->buf + a->prev_offset == old_mem) {
      a->curr_offset = a->prev_offset + new_size;
      if (new_size > old_size) {
        memset(&a->buf[a->curr_offset], 0, new_size - old_size);
      }
      return old_mem;
    } else {
      U8 *new_mem = alloc(a, new_size, align);
      size_t copy_size = old_size < new_size ? old_size : new_size;

      memmove(new_mem, old_mem, copy_size);

      return new_mem;
    }

  } else {
    assert(0 && "Memory is out of bounds of the buffer in this arena");
    return nullptr;
  }
}

void mem::arena::reset(Arena &a) {
  a.curr_offset = 0;
  a.prev_offset = 0;
}
