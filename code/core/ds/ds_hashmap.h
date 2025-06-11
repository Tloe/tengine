#pragma once

#include "arena.h"
#include "ds_string.h"
#include "fnv-1a/fnv.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <types.h>
#include <utility>
#include <vulkan/vulkan_core.h>

// https://thenumb.at/Hashtables - based on robin_hood_with_deletion algorithm

template <typename K, K empty_value, typename V>
struct THashMap {
  using HasherFn = U64 (*)(const K&);

  U64 _size     = 0;
  U64 _capacity = 0;

  struct KeyValue {
    K k;
    V v;
  };

  KeyValue*   _data;
  ArenaHandle _arena_handle;
  HasherFn    _hasher_fn;
};

template <typename K, K empty_value, typename V>
using HashMap = THashMap<K, empty_value, V>;

template <typename V>
using HashMap8 = THashMap<U8, U8_MAX, V>;

template <typename V>
using HashMap16 = THashMap<U16, U16_MAX, V>;

template <typename V>
using HashMap32 = THashMap<U32, U32_MAX, V>;

template <typename V>
using HashMap64 = THashMap<U64, U64_MAX, V>;

template <typename V>
using HashMapString = THashMap<String, String{}, V>;

namespace hashmap {
  // hasher functions
  template <typename K>
  U64 hasher(const K k) {
    return fnv_64a_buf((void*)(&k), sizeof(k), FNV1A_64_INIT);
  }

  template <>
  inline U64 hasher(const String& s) {
    U64 v = fnv_64a_buf(s._data, s._size, FNV1A_64_INIT);
    return v;
  }

  template <>
  inline U64 hasher(const char* s) {
    U64 v = fnv_64a_buf(const_cast<char*>(s), strlen(s), FNV1A_64_INIT);
    return v;
  }

  // HashMap functions
  template <typename K, K empty_value, typename V>
  THashMap<K, empty_value, V>
  init(ArenaHandle                                    arena_handle,
       U64                                            capacity  = 8,
       typename THashMap<K, empty_value, V>::HasherFn hasher_fn = hasher<const K>);

  template <typename V>
  HashMap8<V> init8(ArenaHandle                    arena_handle,
                    U64                            capacity  = 8,
                    typename HashMap8<V>::HasherFn hasher_fn = hasher<const U8&>);

  template <typename V>
  HashMap16<V> init16(ArenaHandle                     arena_handle,
                      U64                             capacity  = 8,
                      typename HashMap16<V>::HasherFn hasher_fn = hasher<const U16&>);

  template <typename V>
  HashMap32<V> init32(ArenaHandle                     arena_handle,
                      U64                             capacity  = 8,
                      typename HashMap32<V>::HasherFn hasher_fn = hasher<const U32&>);

  template <typename V>
  HashMap64<V> init64(ArenaHandle                     arena_handle,
                      U64                             capacity  = 8,
                      typename HashMap64<V>::HasherFn hasher_fn = hasher<const U64&>);

  template <typename V>
  HashMapString<V>
  initString(ArenaHandle                         arena_handle,
             U64                                 capacity  = 8,
             typename HashMapString<V>::HasherFn hasher_fn = hasher<const String&>);

  // template <typename K, K empty_value, typename V>
  // bool initialized(THashMap<K, empty_value, V>& hm);

  template <typename K, K empty_value, typename V>
  void _grow(THashMap<K, empty_value, V>& hm);

  template <typename K, K empty_value, typename V>
  void clear(THashMap<K, empty_value, V>& hm);

  template <typename K, K empty_value, typename V, typename KVar, typename VVar>
  V* insert(THashMap<K, empty_value, V>& hm, const KVar& kvar, const VVar& vvar);

  template <typename K, K empty_value, typename V, typename KVar>
  bool contains(const THashMap<K, empty_value, V>& hm, const KVar& kvar);

  template <typename K, K empty_value, typename V, typename KVar>
  V* value(const THashMap<K, empty_value, V>& hm, const KVar& kvar);

  template <typename K, K empty_value, typename V, typename KVar>
  void erase(THashMap<K, empty_value, V>& hm, const KVar& kvar);

  template <typename K, K empty_value, typename V>
  void for_each(THashMap<K, empty_value, V>& hm,
                void (*fn)(typename THashMap<K, empty_value, V>::KeyValue));
}

// helpers
namespace {

  template <typename K, K empty_value, typename V>
  inline U64 hash_index(const THashMap<K, empty_value, V>& hm, const K& k) {
    U64 hash  = hm._hasher_fn(k);
    U64 index = hash & (hm._capacity - 1);
    return index;
  }

  template <typename K, K empty_value, typename V>
  void _remove(THashMap<K, empty_value, V>& hm, U64 index) {
    for (;;) {
      hm._data[index] = typename THashMap<K, empty_value, V>::KeyValue{.k = empty_value, .v = V{}};

      U64 next = (index + 1) & (hm._capacity - 1);

      if (hm._data[next].k == empty_value) return;

      U64 desired = hash_index(hm, hm._data[next].k);

      if (next == desired) return;

      hm._data[index].k = hm._data[next].k;
      hm._data[index].v = hm._data[next].v;

      index = next;
    }
  }

  template <typename K, K empty_value, typename V>
  inline void _fill_empty_values(THashMap<K, empty_value, V>& hm) {
    for (U32 i = 0; i < hm._capacity; ++i) {
      hm._data[i] = typename THashMap<K, empty_value, V>::KeyValue{
          .k = empty_value,
          .v = V{},
      };
    }
  }

  template <typename K, K empty_value, typename V>
  bool _array_index(const THashMap<K, empty_value, V>& hm, const K& k, U32& array_index) {
    U64 hash          = hm._hasher_fn(k);
    U64 current_index = hash & (hm._capacity - 1);
    U64 dist          = 0;

    for (;;) {
      if (hm._data[current_index].k == empty_value) {
        return false;
      }

      if (hm._data[current_index].k == k) {
        array_index = current_index;
        return true;
      }

      U64 desired   = hash_index(hm, hm._data[current_index].k);
      U64 curr_dist = (current_index + hm._capacity - desired) & (hm._capacity - 1);
      if (curr_dist < dist) {
        return false;
      };

      dist++;
      current_index = (current_index + 1) & (hm._capacity - 1);
    }
  }
}

// implementation
namespace hashmap {
  template <typename K, K empty_value, typename V>
  THashMap<K, empty_value, V> init(ArenaHandle                                    arena_handle,
                                   U64                                            capacity,
                                   typename THashMap<K, empty_value, V>::HasherFn hasher_fn) {
    assert((capacity & (capacity - 1)) == 0 && "HashMap capacity must be power of two");

    THashMap<K, empty_value, V> hm{
        ._size     = 0,
        ._capacity = capacity,
        ._data     = arena::alloc<typename THashMap<K, empty_value, V>::KeyValue>(
            arena_handle,
            capacity * sizeof(typename THashMap<K, empty_value, V>::KeyValue)),
        ._arena_handle = arena_handle,
        ._hasher_fn    = hasher,
    };

    _fill_empty_values<K, empty_value, V>(hm);

    return hm;
  }

  template <typename V>
  HashMap8<V>
  init8(ArenaHandle arena_handle, U64 capacity, typename HashMap8<V>::HasherFn hasher_fn) {
    return init<U8, U8_MAX, V>(arena_handle, capacity, hasher_fn);
  }

  template <typename V>
  HashMap16<V>
  init16(ArenaHandle arena_handle, U64 capacity, typename HashMap16<V>::HasherFn hasher_fn) {
    return init<U16, U16_MAX, V>(arena_handle, capacity, hasher_fn);
  }

  template <typename V>
  HashMap16<V>
  init32(ArenaHandle arena_handle, U64 capacity, typename HashMap32<V>::HasherFn hasher_fn) {
    return init<U32, U32_MAX, V>(arena_handle, capacity, hasher_fn);
  }

  template <typename V>
  HashMap64<V>
  init64(ArenaHandle arena_handle, U64 capacity, typename HashMap64<V>::HasherFn hasher_fn) {
    return init<U64, U64_MAX, V>(arena_handle, capacity, hasher_fn);
  }

  template <typename V>
  HashMapString<V> initString(ArenaHandle                         arena_handle,
                              U64                                 capacity,
                              typename HashMapString<V>::HasherFn hasher_fn) {
    return init<String, String{}, V>(arena_handle, capacity, hasher_fn);
  }

  // template <typename K, K empty_value, typename V>
  // inline bool initialized(THashMap<K, empty_value, V>& hm) {
  //   return (hm._capacity != 0);
  // }

  template <typename K, K empty_value, typename V>
  void _grow(THashMap<K, empty_value, V>& hm) {
    U64  old_capacity = hm._capacity;
    auto old_data     = hm._data;
    hm._size          = 0;
    hm._capacity *= 2;

    hm._data = arena::alloc<typename THashMap<K, empty_value, V>::KeyValue>(
        hm._arena_handle,
        hm._capacity * sizeof(typename THashMap<K, empty_value, V>::KeyValue));

    assert(hm._data != nullptr);

    _fill_empty_values<K, empty_value, V>(hm);

    for (U64 i = 0; i < old_capacity; i++) {
      if (old_data[i].k != empty_value) insert(hm, old_data[i].k, old_data[i].v);
    }
  }

  template <typename K, K empty_value, typename V>
  void clear(THashMap<K, empty_value, V>& hm) {
    hm._size = 0;
    _fill_empty_values<K, empty_value, V>(hm);
  }

  template <typename K, K empty_value, typename V, typename KVar>
  V* insert(THashMap<K, empty_value, V>& hm, const KVar& kvar) {
    return insert(hm, kvar, V{});
  }

  template <typename K, K empty_value, typename V, typename KVar, typename VVar>
  V* insert(THashMap<K, empty_value, V>& hm, const KVar& kvar, const VVar& vvar) {
    assert(hm._capacity != 0 && "init not called");

    if (hm._size >= hm._capacity) {
      _grow(hm);
    }

    K k = static_cast<K>(kvar);
    V v = static_cast<V>(vvar);

    V* inserted = nullptr;

    U64 hash  = hm._hasher_fn(k);
    U64 index = hash & (hm._capacity - 1);
    U64 dist  = 0;
    hm._size++;

    for (;;) {
      if (hm._data[index].k == empty_value) {
        hm._data[index].k = k;
        hm._data[index].v = v;

        if (inserted == nullptr) {
          inserted = &hm._data[index].v;
        }

        break;
      }

      U64 desired = hash_index(hm, hm._data[index].k);

      U64 curr_dist = (index + hm._capacity - desired) & (hm._capacity - 1);
      if (curr_dist < dist) {
        std::swap(k, hm._data[index].k);
        std::swap(v, hm._data[index].v);

        if (inserted == nullptr) {
          inserted = &hm._data[index].v;
        }

        dist = curr_dist;
      }
      dist++;
      index = (index + 1) & (hm._capacity - 1);
    }

    return inserted;
  }

  template <typename K, K empty_value, typename V, typename KVar>
  bool contains(const ::THashMap<K, empty_value, V>& hm, const KVar& kvar) {
    U32 out_not_used;
    return _array_index(hm, static_cast<K>(kvar), out_not_used);
  }

  template <typename K, K empty_value, typename V, typename KVar>
  V* value(const THashMap<K, empty_value, V>& hm, const KVar& kvar) {
    U32 array_index;
    if (!_array_index(hm, static_cast<K>(kvar), array_index)) {
      return nullptr;
    }

    return &hm._data[array_index].v;
  }

  template <typename K, K empty_value, typename V, typename KVar>
  void erase(THashMap<K, empty_value, V>& hm, const KVar& kvar) {
    K k = kvar;

    if (!contains(hm, k)) {
      return;
    }

    U64 hash  = hm._hasher_fn(k);
    U64 index = hash & (hm._capacity - 1);
    for (;;) {
      if (hm._data[index].k == k) {
        _remove(hm, index);
        --hm._size;
        return;
      }
      index = (index + 1) & (hm._capacity - 1);
    }
  }

  template <typename K, K empty_value, typename V>
  void for_each(THashMap<K, empty_value, V>& hm,
                void (*fn)(typename THashMap<K, empty_value, V>::KeyValue)) {
    for (U32 i = 0; i < hm._capacity; ++i) {
      if (hm._data[i].k != empty_value) {
        fn(hm._data[i]);
      }
    }
  }
}
