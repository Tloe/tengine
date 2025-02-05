#ifndef DARRAY_H
#define DARRAY_H

#include "arena.h"
#include "types.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <utility>

namespace ds {
  template <typename T> struct DynamicArray {
    U32 _size = 0;
    U32 _capacity = 0;
    T *_data = nullptr;
    mem::Arena *_a;
  };

  namespace array {
    template <typename T> DynamicArray<T> init(mem::Arena *a, U32 capacity = 8, U32 size = 0);
    template <typename T> DynamicArray<T> init(mem::Arena *a, const T *beg, const T *end);
    template <typename T> void reserve(DynamicArray<T> &da, U32 new_capacity);
    template <typename T> void resize(DynamicArray<T> &da, U32 size);
    template <typename T> T &push_back(DynamicArray<T> &da, const T &t);
    template <typename T> T &push_back(DynamicArray<T> &da, T &&t);
    template <typename T> T &push_back_unique(DynamicArray<T> &da, const T &t);
    template <typename T> T &push_back_unique(DynamicArray<T> &da, T &&t);
    template <typename T> bool contains(DynamicArray<T> &da, const T &t);
    template <typename T> bool contains(DynamicArray<T> &da, T &t);
  }
}

namespace ds {
  namespace array {
    template <typename T> ds::DynamicArray<T> init(mem::Arena *a, U32 capacity, U32 size) {
      assert(size <= capacity && "init size is larger than capacity");
      DynamicArray<T> da{
          ._size = size,
          ._capacity = capacity,
          ._data = mem::arena::alloc<T>(a, sizeof(T) * capacity),
          ._a = a,
      };

      return da;
    }

    template <typename T> ds::DynamicArray<T> init(mem::Arena *a, const T *beg, const T *end) {
      printf("AAAAAAA %d", end - beg);
      DynamicArray<T> da{
          ._size = end - beg,
          ._capacity = end - beg,
          ._data = mem::arena::alloc<T>(a, sizeof(T) * end - beg),
          ._a = a,
      };

      memcpy(da._data, beg, sizeof(T) * end - beg);
      return da;
    }

    template <typename T> void reserve(DynamicArray<T> &da, U32 new_capacity) {
      assert(da._data == nullptr && "use init for first reserve");

      if (da._capacity < new_capacity) {
        auto old_data = da._data;
        da._data = mem::arena::alloc<T>(da._a, sizeof(T) * new_capacity);
        memcpy(da._data, old_data, da._size);
        da._capacity = new_capacity;
      }
    }

    template <typename T> void resize(DynamicArray<T> &da, U32 size) {
      if (da._size < size) {
        if (da._capacity < size) reserve(da, size);

        for (U32 i = da._size; i < size; i++) {
          da._data[i] = T{};
        }
      }
      da._size = size;
    }

    template <typename T> T &push_back(DynamicArray<T> &da, const T &t) {
      if (da._size == da._capacity) {
        reserve(da, da._capacity * 2);
      }

      return da._data[++da._size] = t;
    }

    template <typename T> T &push_back(DynamicArray<T> &da, T &t) {
      if (da._size == da._capacity) {
        reserve(da, da._capacity * 2);
      }

      return da._data[++da._size] = t;
    }

    template <typename T> T &push_back_unique(DynamicArray<T> &da, const T &t) {
      if (contains(da, t)) return;

      push_back(da, t);
    }

    template <typename T> T &push_back_unique(DynamicArray<T> &da, T &t) {
      if (contains(da, t)) return;

      push_back(da, t);
    }

    template <typename T> bool contains(DynamicArray<T> &da, const T &t) {
      for (int i = 0; i < da._size; i++) {
        if (da._data[i] == t) {
          return true;
        }
      }

      return false;
    }

    template <typename T> bool contains(DynamicArray<T> &da, T &t) {
      for (int i = 0; i < da._size; i++) {
        if (da._data[i] == t) {
          return true;
        }
      }

      return false;
    }
  }
}

#endif
