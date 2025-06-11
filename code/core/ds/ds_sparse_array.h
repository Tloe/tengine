#pragma once

#include "ds_array_dynamic.h"
#include "ds_array_static.h"
#include "handles.h"
#include "types.h"

#include <cstdio>

template <typename T, typename LType, U32 MaxInstances, U32 MaxAvailable>
struct TStaticSparseArray {
  StaticArray<T, MaxInstances>     _data;
  StaticArray<LType, MaxAvailable> __lookup;
  StaticArray<LType, MaxAvailable> __reverse_lookup;
  U32                              _size;
  LType                            __next_id;
};

template <typename T, U32 MI, U32 MA>
using StaticSparseArray8 = TStaticSparseArray<T, U8, MI, MA>;

template <typename T, U32 MI, U32 MA>
using StaticSparseArray16 = TStaticSparseArray<T, U16, MI, MA>;

template <typename T, U32 MI, U32 MA>
using StaticSparseArray32 = TStaticSparseArray<T, U32, MI, MA>;

namespace sparse {
  template <typename T, typename LType, U32 MI, U32 MA>
  TStaticSparseArray<T, LType, MI, MA> init(ArenaHandle arena) {
    auto sa = TStaticSparseArray<T, LType, MI, MA>{
        ._data            = array::init<T, MI>(arena),
        .__lookup         = array::init<LType, MA>(arena, LType(-1)),
        .__reverse_lookup = array::init<LType, MA>(arena, LType(-1)),
        ._size            = 0,
        .__next_id        = 0,
    };

    return sa;
  }

  template <typename T, U32 MI, U32 MA>
  StaticSparseArray8<T, MI, MA> init8(ArenaHandle arena) {
    return init<T, U8, MI, MA>(arena);
  }

  template <typename T, U32 MI, U32 MA>
  StaticSparseArray16<T, MI, MA> init16(ArenaHandle arena) {
    return init<T, U16, MI, MA>(arena);
  }

  template <typename T, U32 MI, U32 MA>
  StaticSparseArray32<T, MI, MA> init32(ArenaHandle arena) {
    return init<T, U32, MI, MA>(arena);
  }

  template <typename T, typename LType, U32 MI, U32 MA>
  bool insert(TStaticSparseArray<T, LType, MI, MA>& sa, LType id, const T& value) {
    if (id >= MA || sa._size >= MI) {
      return false;
    }

    if (sa.__lookup.data[id] != LType(-1)) {
      sa._data.data[sa.__lookup.data[id]] = value;
    } else {
      U32 data_index                       = sa._size++;
      sa.__lookup.data[id]                 = data_index;
      sa.__reverse_lookup.data[data_index] = id;
      sa._data.data[data_index]            = value;
    }

    return true;
  }

  template <typename T, typename LType, U32 MI, U32 MA>
  void remove(TStaticSparseArray<T, LType, MI, MA>& sa, LType id) {
    if (id >= MA || sa.__lookup.data[id] == LType(-1)) return;

    U32 data_index = sa.__lookup.data[id];
    U32 last_index = sa._size - 1;

    if (data_index != last_index) {
      sa._data.data[data_index] = sa._data.data[last_index];

      LType moved_id                       = sa.__reverse_lookup.data[last_index];
      sa.__reverse_lookup.data[data_index] = moved_id;

      sa.__lookup.data[moved_id] = data_index;
    }

    sa._size--;
    sa.__lookup.data[id]                 = LType(-1);
    sa.__reverse_lookup.data[last_index] = LType(-1);
    sa._data.data[last_index]            = T{};
  }

  template <typename T, typename LType, U32 MI, U32 MA>
  bool has(const TStaticSparseArray<T, LType, MI, MA>& sa, LType id) {
    return id < MA && sa.__lookup.data[id] != LType(-1);
  }

  template <typename T, typename LType, U32 MI, U32 MA>
  T* value(TStaticSparseArray<T, LType, MI, MA>& sa, LType id) {
    if (id < MA && sa.__lookup.data[id] != LType(-1)) {
      return &sa._data.data[sa.__lookup.data[id]];
    } else {
      return nullptr;
    }
  }

  template <typename T, typename LType, U32 MI, U32 MA>
  LType next_id(TStaticSparseArray<T, LType, MI, MA>& sa) {
    if (sa._size >= MI) return core::max_type<LType>();

    U32   start_id = sa.__next_id;
    LType next_id  = start_id;

    do {
      if (sa.__lookup.data[next_id] == LType(-1)) {
        sa.__next_id = (next_id + 1) % MA;
        return next_id;
      }

      next_id = (next_id + 1) % MA;

    } while (next_id != start_id);

    return core::max_type<LType>();
  }
}
