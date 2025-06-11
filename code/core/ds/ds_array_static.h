#pragma once

#include "arena.h"
#include "handles.h"
#include "types.h"

#include <cassert>
#include <cstddef>

template <typename T, U32 SIZE>
struct StaticArray {
  U32 size = SIZE;
  T*  data = nullptr;
};

namespace array {
  template <typename T, U32 SIZE>
  StaticArray<T, SIZE> init(ArenaHandle arena_handle, T init_value = T{}) {
    auto sa = StaticArray<T, SIZE>{
        .size = SIZE,
        .data = arena::alloc<T>(arena_handle, sizeof(T) * SIZE),
    };

    for (U32 i = 0; i < SIZE; ++i) {
      sa.data[i] = init_value;
    }

    return sa;
  }

  template <typename T, U32 SIZE>
  U32 bytes(StaticArray<T, SIZE>& sa) {
    return sizeof(sa.data) * sa.size;
  }

  template <typename T, U32 SIZE>
  U32 element_bytes(StaticArray<T, SIZE> sa) {
    return sizeof(sa.data);
  }

  // native arrays
  template <typename T, U32 SIZE>
  const U32 size(const T (&array)[SIZE]) {
    return SIZE;
  }

  template <typename T, size_t SIZE>
  U32 bytes(const T (&array)[SIZE]) {
    return SIZE * sizeof(array[0]);
  }

  template <typename T>
  U32 element_bytes(const T* array) {
    return sizeof(array[0]);
  }
}
