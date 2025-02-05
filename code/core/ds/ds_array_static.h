#ifndef SARRAY_H
#define SARRAY_H

#include "types.h"

namespace mem { struct Arena; }

namespace ds {
  template <typename T,U32 SIZE> struct StaticArray {
    const U32 size = SIZE;
    T data[SIZE];
  };

  namespace array {
    template <typename T, U32 SIZE> StaticArray<T, SIZE> init(mem::Arena *a, U32 capacity = 8);
    template <typename T, U32 SIZE> U32 bytes(StaticArray<T, SIZE> &sa) {
      return sizeof(sa.data) * sa.size;
    }

    template <typename T, U32 SIZE> U32 element_bytes(StaticArray<T, SIZE> sa) {
      return sizeof(sa.data);
    }
  }
}

#endif
