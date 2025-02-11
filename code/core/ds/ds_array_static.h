#ifndef SARRAY_H
#define SARRAY_H

#include "arena.h"
#include "types.h"
#include <cassert>
#include <cstddef>

namespace mem {
  struct Arena;
}

namespace ds {
  template <typename T, U32 SIZE> struct StaticArray {
    const U32 size = SIZE;
    T *data = nullptr;
  };

  namespace array {
    template <typename T, U32 SIZE> StaticArray<T, SIZE> init(mem::Arena *a) {
      return StaticArray<T, SIZE>{
          .size = SIZE,
          .data = mem::arena::alloc<T>(a, sizeof(T) * SIZE),
      };
    }

    template <typename T, U32 SIZE> U32 bytes(StaticArray<T, SIZE> &sa) {
      return sizeof(sa.data) * sa.size;
    }

    template <typename T, U32 SIZE> U32 element_bytes(StaticArray<T, SIZE> sa) {
      return sizeof(sa.data);
    }

    // native arrays
    template <typename T, size_t SIZE> U32 size(const T (&array)[SIZE]) {
      return SIZE;
    }

    template <typename T, size_t SIZE> U32 bytes(const T (&array)[SIZE]) {
      return SIZE * sizeof(array[0]);
    }

    template <typename T> U32 element_bytes(const T* array) {
      return sizeof(array[0]);
    }
  }
}

#endif
