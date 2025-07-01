#include "ds_array_dynamic.h"

template <typename T, typename LType>
struct DynamicSparseArray {
  DynamicArray<T>     _data;
  DynamicArray<LType> __lookup;
  DynamicArray<LType> __reverse_lookup;
  U32                 _size;
  LType               __next_id;
  LType               _max_instance;
};

namespace sparse {
  template <typename T, typename LType>
  DynamicSparseArray<T, LType> init(ArenaHandle arena, LType max_instance, U32 initial_capacity = 16) {
    DynamicSparseArray<T, LType> sa{
      ._data            = array::init<T>(arena, initial_capacity),
      .__lookup         = array::init<LType>(arena, initial_capacity, max_instance),
      .__reverse_lookup = array::init<LType>(arena, initial_capacity, max_instance),
      ._size            = 0,
      .__next_id        = 0,
      ._max_instance    = max_instance,
    };
    return sa;
  }

  template <typename T, typename LType>
  bool insert(DynamicSparseArray<T, LType>& sa, LType id, const T& value) {
    if (id >= sa.__lookup._capacity) {
      array::resize(sa.__lookup, id + 1);
      for (U32 i = sa.__lookup._size; i < sa.__lookup._capacity; ++i)
        sa.__lookup._data[i] = sa._max_instance;
      array::resize(sa.__reverse_lookup, id + 1);
      for (U32 i = sa.__reverse_lookup._size; i < sa.__reverse_lookup._capacity; ++i)
        sa.__reverse_lookup._data[i] = sa._max_instance;
    }
    if (sa._size >= sa._data._capacity) {
      array::resize(sa._data, sa._size + 1);
    }

    if (sa.__lookup._data[id] != sa._max_instance) {
      sa._data._data[sa.__lookup._data[id]] = value;
    } else {
      U32 data_index                       = sa._size++;
      sa.__lookup._data[id]                = data_index;
      sa.__reverse_lookup._data[data_index] = id;
      sa._data._data[data_index]           = value;
    }
    return true;
  }

  template <typename T, typename LType>
  void remove(DynamicSparseArray<T, LType>& sa, LType id) {
    if (id >= sa.__lookup._capacity || sa.__lookup._data[id] == sa._max_instance) return;

    U32 data_index = sa.__lookup._data[id];
    U32 last_index = sa._size - 1;

    if (data_index != last_index) {
      sa._data._data[data_index] = sa._data._data[last_index];

      LType moved_id                        = sa.__reverse_lookup._data[last_index];
      sa.__reverse_lookup._data[data_index] = moved_id;

      sa.__lookup._data[moved_id] = data_index;
    }

    sa._size--;
    sa.__lookup._data[id]                 = sa._max_instance;
    sa.__reverse_lookup._data[last_index] = sa._max_instance;
    sa._data._data[last_index]            = T{};
  }

  template <typename T, typename LType>
  bool has(const DynamicSparseArray<T, LType>& sa, LType id) {
    return id < sa.__lookup._capacity && sa.__lookup._data[id] != sa._max_instance;
  }

  template <typename T, typename LType>
  T* value(DynamicSparseArray<T, LType>& sa, LType id) {
    if (id < sa.__lookup._capacity && sa.__lookup._data[id] != sa._max_instance) {
      return &sa._data._data[sa.__lookup._data[id]];
    } else {
      return nullptr;
    }
  }

  template <typename T, typename LType>
  LType next_id(DynamicSparseArray<T, LType>& sa) {
    U32 cap = sa.__lookup._capacity;
    if (sa._size >= cap) return core::max_type<LType>();

    U32   start_id = sa.__next_id;
    LType next_id  = start_id;

    do {
      if (next_id >= cap) {
        array::resize(sa.__lookup, next_id + 1);
        for (U32 i = sa.__lookup._size; i < sa.__lookup._capacity; ++i)
          sa.__lookup._data[i] = sa._max_instance;
        array::resize(sa.__reverse_lookup, next_id + 1);
        for (U32 i = sa.__reverse_lookup._size; i < sa.__reverse_lookup._capacity; ++i)
          sa.__reverse_lookup._data[i] = sa._max_instance;
        cap = sa.__lookup._capacity;
      }
      if (sa.__lookup._data[next_id] == sa._max_instance) {
        sa.__next_id = (next_id + 1) % cap;
        return next_id;
      }

      next_id = (next_id + 1) % cap;

    } while (next_id != start_id);

    return core::max_type<LType>();
  }
}

