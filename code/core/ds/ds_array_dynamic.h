#pragma once

#include "arena.h"
#include "handles.h"
#include "types.h"

#include <cassert>
#include <cstring>
#include <initializer_list>

template <typename T>
struct DynamicArray {
  U32         _size     = 0;
  U32         _capacity = 0;
  T*          _data     = nullptr;
  ArenaHandle _arena_handle;
};

namespace array {
  template <typename T>
  DynamicArray<T> init(ArenaHandle a, U32 capacity = 8, U32 size = 0);
  template <typename T>
  DynamicArray<T> init(ArenaHandle a, const T* beg, const T* end);
  template <typename T>
  DynamicArray<T> init(ArenaHandle a, std::initializer_list<T> ilist);
  template <typename T>
  DynamicArray<T> init(ArenaHandle a, const DynamicArray<T>& other);
  template <typename T>
  void reserve(DynamicArray<T>& da, U32 new_capacity);
  template <typename T>
  void resize(DynamicArray<T>& da, U32 size);
  template <typename T>
  T& push_back(DynamicArray<T>& da, const T& t);
  template <typename T>
  T& push_back(DynamicArray<T>& da, T& t);
  template <typename T>
  T& push_back_unique(DynamicArray<T>& da, const T& t);
  template <typename T>
  T& push_back_unique(DynamicArray<T>& da, T& t);
  template <typename T>
  bool contains(DynamicArray<T>& da, const T& t);
  template <typename T>
  bool contains(DynamicArray<T>& da, T& t);
}

#define S_DARRAY(T, ...)       array::init<T>(arena::scratch(), std::initializer_list<T>{__VA_ARGS__})
// #define S_DARRAY_EMPTY(T)      array::init<T>(arena::scratch())
#define S_DARRAY_CAP(T, CAP)   array::init<T>(arena::scratch(), CAP)
#define S_DARRAY_SIZE(T, SIZE) array::init<T>(arena::scratch(), SIZE, SIZE)

#define F_DARRAY(T, ...)       array::init<T>(arena::frame(), std::initializer_list<T>{__VA_ARGS__})
// #define F_DARRAY_EMPTY(T)      array::init<T>(arena::frame())
#define F_DARRAY_CAP(T, CAP)   array::init<T>(arena::frame(), CAP)
#define F_DARRAY_SIZE(T, SIZE) array::init<T>(arena::frame(), SIZE, SIZE)

#define A_DARRAY(T, ARENA, ...)       array::init<T>(ARENA, std::initializer_list<T>{__VA_ARGS__})
#define A_DARRAY_CAP(T, ARENA, CAP)   array::init<T>(ARENA, CAP)
#define A_DARRAY_SIZE(T, ARENA, SIZE) array::init<T>(ARENA, SIZE, SIZE)

namespace array {
  template <typename T>
  DynamicArray<T> init(ArenaHandle a, U32 capacity, U32 size) {
    assert(size <= capacity && "init size is larger than capacity");
    DynamicArray<T> da{
        ._size         = size,
        ._capacity     = capacity,
        ._data         = arena::alloc<T>(a, sizeof(T) * capacity),
        ._arena_handle = a,
    };

    return da;
  }

  template <typename T>
  DynamicArray<T> init(ArenaHandle a, const T* beg, const T* end) {
    U32             size = static_cast<U32>(end - beg);
    DynamicArray<T> da{
        ._size         = size,
        ._capacity     = size,
        ._data         = arena::alloc<T>(a, sizeof(T) * size),
        ._arena_handle = a,
    };

    memcpy(da._data, beg, sizeof(T) * size);

    return da;
  }

  template <typename T>
  DynamicArray<T> init(ArenaHandle a, std::initializer_list<T> ilist) {
    U32 size = static_cast<U32>(ilist.size());

    if (size == 0) {
      return init<T>(a);
    }

    DynamicArray<T> da{
        ._size         = size,
        ._capacity     = size,
        ._data         = arena::alloc<T>(a, sizeof(T) * size),
        ._arena_handle = a,
    };

    memcpy(da._data, ilist.begin(), sizeof(T) * size);

    return da;
  }

  template <typename T>
  DynamicArray<T> init(ArenaHandle a, const DynamicArray<T>& other) {
    DynamicArray<T> da{
        ._size         = other._size,
        ._capacity     = other._size,
        ._data         = arena::alloc<T>(a, sizeof(T) * other._size),
        ._arena_handle = a,
    };

    memcpy(da._data, other._data, sizeof(T) * other._size);

    return da;
  }

  template <typename T>
  void reserve(DynamicArray<T>& da, U32 new_capacity) {
    assert(da._data != nullptr && "use init for first reserve");

    if (da._capacity < new_capacity) {
      T* old_data = da._data;
      da._data    = arena::alloc<T>(da._arena_handle, sizeof(T) * new_capacity);
      memcpy(da._data, old_data, da._size * sizeof(da._data[0]));
      da._capacity = new_capacity;
    }
  }

  template <typename T>
  void resize(DynamicArray<T>& da, U32 size) {
    if (da._size < size) {
      if (da._capacity < size) reserve(da, size);

      for (U32 i = da._size; i < size; i++) {
        da._data[i] = T{};
      }
    }
    da._size = size;
  }

  template <typename T>
  void pop_back(DynamicArray<T>& da) {
    ++da._size;
  }

  template <typename T>
  T& push_back(DynamicArray<T>& da, const T& t) {
    if (da._size == da._capacity) {
      reserve(da, (da._capacity == 0 ? 1 : da._capacity * 2));
    }

    return da._data[da._size++] = t;
  }

  template <typename T>
  T& push_back(DynamicArray<T>& da, T& t) {
    if (da._size == da._capacity) {
      reserve(da, (da._capacity == 0 ? 1 : da._capacity * 2));
    }

    return da._data[da._size++] = t;
  }

  template <typename T>
  T& push_back_unique(DynamicArray<T>& da, const T& t) {
    if (contains(da, t)) return t;

    return push_back(da, t);
  }

  template <typename T>
  T& push_back_unique(DynamicArray<T>& da, T& t) {
    if (contains(da, t)) return t;

    return push_back(da, t);
  }

  template <typename T>
  bool contains(DynamicArray<T>& da, const T& t) {
    for (int i = 0; i < da._size; i++) {
      if (da._data[i] == t) {
        return true;
      }
    }

    return false;
  }

  template <typename T>
  bool contains(DynamicArray<T>& da, T& t) {
    for (int i = 0; i < da._size; i++) {
      if (da._data[i] == t) {
        return true;
      }
    }

    return false;
  }
}
