#include "arena.h"

#include "handles.h"
#include "types.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
  const U8 MAX_ARENA_COUNT = U8_MAX;
  const U8 NUM_FRAMES      = 2;
  U8       _current_frame  = 0;

  ArenaHandle _frames[2] = {
      arena::by_name("frame0"),
      arena::by_name("frame1"),
  };

  struct {
    const char* name = "";
    ArenaHandle handle;
    Arena       a;
  } _arenas[MAX_ARENA_COUNT] = {{}};

  bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

  U32 align_forward(U8* ptr, U8 align) {
    auto p = reinterpret_cast<uintptr_t>(ptr);

    assert(is_power_of_two(align));

    auto modulo = p & (align - 1);

    if (modulo != 0) {
      p += align - modulo;
    }
    return p;
  }
}

ArenaHandle arena::set(const char* name, U8* mem, U32 mem_size) {
  ArenaHandle handle = by_name(name);

  _arenas[handle.value].name          = name;
  _arenas[handle.value].a.buf         = mem;
  _arenas[handle.value].a.buf_len     = mem_size;
  _arenas[handle.value].a.curr_offset = 0;
  _arenas[handle.value].a.prev_offset = 0;

  return handle;
}

void arena::set_scratch(U8* mem, U32 size) {
  _arenas[0].name          = "scratch";
  _arenas[0].a.buf         = mem;
  _arenas[0].a.buf_len     = size;
  _arenas[0].a.curr_offset = 0;
  _arenas[0].a.prev_offset = 0;
}

ArenaHandle arena::by_name(const char* name) {
  for (U8 i = 1; i < MAX_ARENA_COUNT; ++i) {
    if (_arenas[i].name[0] == '\0') break;

    if (strcmp(_arenas[i].name, name) == 0) {
      return ArenaHandle{.value = i};
    }
  }

  for (U8 i = 1; i < MAX_ARENA_COUNT; ++i) {
    if (_arenas[i].name[0] == '\0') {
      _arenas[i].name = name;
      return ArenaHandle{.value = i};
    }
  }

  assert(false && "ran out of arena allocators!");
}

ArenaHandle arena::frame() { return _frames[_current_frame]; }

void arena::next_frame() {
  ++_current_frame;
  _current_frame %= NUM_FRAMES;
  reset(_frames[_current_frame]);
}

U8* arena::alloc(ArenaHandle handle, U32 size, U8 align) {
  if (_arenas[handle.value].a.buf_len == 0) {
    printf("arena '%s' has not been set up", _arenas[handle.value].name);
    exit(0);
  }
  auto a = &_arenas[handle.value].a;

  auto curr_ptr = a->buf + a->curr_offset;
  U32  offset   = align_forward(curr_ptr, align);
  offset -= reinterpret_cast<uintptr_t>(a->buf);

  if (offset + size <= a->buf_len) {
    U8* ptr        = &a->buf[offset];
    a->prev_offset = offset;
    a->curr_offset = offset + size;

    memset(ptr, 0, size);

    return ptr;
  }

  printf("Memory is out of bounds of the buffer in this arena. Name of arena: '%s'\n",
         _arenas[handle.value].name);
  assert(false);
  return nullptr;
}

U8* arena::resize(ArenaHandle handle, U8* old_mem, U32 old_size, U32 new_size, U8 align) {
  assert(is_power_of_two(align));

  auto a = &_arenas[handle.value].a;

  if (old_mem == nullptr || old_size == 0) {
    return alloc(handle, new_size, align);
  } else if (a->buf <= old_mem && old_mem < a->buf + a->buf_len) {
    if (a->buf + a->prev_offset == old_mem) {
      a->curr_offset = a->prev_offset + new_size;
      if (new_size > old_size) {
        memset(&a->buf[a->curr_offset], 0, new_size - old_size);
      }
      return old_mem;
    } else {
      U8*    new_mem   = alloc(handle, new_size, align);
      size_t copy_size = old_size < new_size ? old_size : new_size;

      memmove(new_mem, old_mem, copy_size);

      return new_mem;
    }

  } else {
    assert(0 && "Memory is out of bounds of the buffer in this arena");
    return nullptr;
  }
}

void arena::reset(ArenaHandle handle) {
  auto a = &_arenas[handle.value].a;

  a->curr_offset = 0;
  a->prev_offset = 0;
}
