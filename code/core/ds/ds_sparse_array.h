#pragma once

#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "types.h"

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
struct TStaticSparseArray {
  StaticArray<T, MaxInstances> _data;
  StaticArray<LType, MaxAvailable> __lookup;
  StaticArray<T, MaxAvailable> __reverse_lookup;
  U32 _size;
  LType __next_id;
};

template <typename T, U32 MI, U32 MA>
using StaticSparseArray8 = TStaticSparseArray<T, U8, MI, MA>;

template <typename T, U32 MI, U32 MA>
using StaticSparseArray16 = TStaticSparseArray<T, U16, MI, MA>;

template <typename T, U32 MI, U32 MA>
using StaticSparseArray32 = TStaticSparseArray<T, U32, MI, MA>;

namespace array {
template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
TStaticSparseArray<T, LType, MaxInstances, MaxAvailable>
init(ArenaHandle arena_handle, LType id, const T &value) {
  TStaticSparseArray<T, LType, MaxInstances, MaxAvailable> sa{
      ._data = init<T, MaxInstances>(arena_handle),
      .__lookup = init<LType, MaxAvailable>(arena_handle),
      .__reverse_lookup = init<T, MaxAvailable>(arena_handle),
      ._size = 0,
      .__next_id = 0,
  };
}

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
bool insert(TStaticSparseArray<T, LType, MaxInstances, MaxAvailable> &sa,
            LType id, T &value) {
  if (id >= MaxAvailable || sa._size >= MaxInstances) {
    return false;
  }

  if (sa.__lookup.data[id] != -1) {
    sa._data[sa.__lookup[id]] = value;
  } else {
    U32 data_index = sa._size++;
    sa.__lookup[id] = data_index;
    sa.__reverse_lookup[data_index] = id;
    sa.data[data_index] = value;
  }

  return true;
}

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
void remove(TStaticSparseArray<T, LType, MaxInstances, MaxAvailable> &sa,
            LType id) {
  if (id >= MaxAvailable || sa.__reverse_lookup[id] == -1)
    return;

  U32 data_index = sa.__lookup[id];
  U32 last_index = sa._size - 1;

  if (data_index != last_index) {
    sa._data[data_index] = sa._data[last_index];
    sa.__reverse_lookup[data_index] = sa.__reverse_lookup[last_index];
    sa.__lookup[sa.__reverse_lookup[data_index]] = data_index;
  }

  sa._size--;
  sa.__lookup[id] = -1;
}

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
T *value(TStaticSparseArray<T, LType, MaxInstances, MaxAvailable> &sa,
         LType id) {
  if (id < MaxAvailable && sa.__lookup[id] != -1) {
    return &sa._data[sa.__lookup[id]];
  } else {
    return nullptr;
  }
}

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
LType next_id(TStaticSparseArray<T, LType, MaxInstances, MaxAvailable> &sa) {
  if (sa.count >= MaxInstances)
    return U32_MAX;

  U32 start_id = sa.__next_id;
  LType next_id = start_id;

  do {
    if (sa.__lookup[next_id] == -1) {
      sa.__next_id = (next_id + 1) % MaxAvailable;
      return next_id;
    }

    next_id = (next_id + 1) % MaxAvailable;

  } while (next_id != start_id);

  return core::max_type<LType>();
}
} // namespace array
