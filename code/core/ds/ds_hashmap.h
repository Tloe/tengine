#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"
#include "ds_string.h"
#include "fnv-1a/fnv.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <types.h>
#include <utility>

// https://thenumb.at/Hashtables - based on robin_hood_with_deletion algorithm

namespace ds {
  template <typename K, typename V>
  struct HashMap {
    U64 _size     = 0;
    U64 _capacity = 0;

    struct _KeyValue {
      K k;
      V v;
    };

    _KeyValue*    _data;
    mem::ArenaPtr _a;
  };

  namespace hashmap {
    template <typename K, typename V>
    HashMap<K, V> init(mem::Arena* a, U64 capacity = 8);

    template <typename K, typename V>
    void grow(HashMap<K, V>& hm);

    template <typename K, typename V>
    void clear(HashMap<K, V>& hm);

    template <typename K, typename V>
    V* insert(HashMap<K, V>& hm, const K& k, const V& v);

    template <typename K, typename V>
    bool contains(const HashMap<K, V>& hm, const K& k);

    template <typename K, typename V>
    V* value(const HashMap<K, V>& hm, const K& k);

    template <typename K, typename V>
    void erase(HashMap<K, V>& hm, const K& k);
  }
}

// helpers
namespace {
  template <typename K>
  K empty_value() {
    assert(false && "implement more HashMap empty_value specializations");
    return K{};
  }

  template <>
  inline ds::String empty_value() {
    return ds::String{};
  }
  template <>
  inline U8 empty_value() {
    return U8_MAX;
  }
  template <>
  inline U16 empty_value() {
    return U16_MAX;
  }
  template <>
  inline U32 empty_value() {
    return U32_MAX;
  }
  template <>
  inline U64 empty_value() {
    return U64_MAX;
  }

  inline U64 _hash_key(ds::String s) {
    U64 v = fnv_64a_buf(s._data, s._size, FNV1A_64_INIT);
    return v;
  }

  inline U64 _hash_key(U64 ui) {
    return fnv_64a_buf(reinterpret_cast<void*>(&ui), sizeof(ui), FNV1A_64_INIT);
  }

  template <typename K, typename V>
  inline U64 _index_for_key(const ds::HashMap<K, V>& hm, const K& k) {
    U64 hash  = _hash_key(k);
    U64 index = hash & (hm._capacity - 1);
    return index;
  }

  template <typename K, typename V>
  void _remove(ds::HashMap<K, V>& hm, U64 index) {
    for (;;) {
      hm._data[index].k = empty_value<K>();

      U64 next = (index + 1) & (hm._capacity - 1);

      if (hm._data[next].k == empty_value<K>()) return;

      U64 desired = _index_for_key(hm, hm._data[next].k);

      if (next == desired) return;

      hm._data[index].k = hm._data[next].k;
      hm._data[index].v = hm._data[next].v;

      index = next;
    }
  }

  template <typename K, typename V>
  void _fill_empty_values(ds::HashMap<K, V>& hm) {
    for (U32 i = 0; i < hm._capacity; ++i) {
      hm._data[i] = typename ds::HashMap<K, V>::_KeyValue{
          .k = empty_value<K>(),
          .v = V{},
      };
    }
  }

  template <typename K, typename V>
  bool array_index(const ds::HashMap<K, V>& hm, const K& k, U32& array_index) {
    U64 hash          = _hash_key(k);
    U64 current_index = hash & (hm._capacity - 1);
    U64 dist          = 0;
    for (;;) {
      if (hm._data[current_index].k == empty_value<K>()) {
        return false;
      }

      if (hm._data[current_index].k == k) {
        array_index = current_index;
        return true;
      }

      U64 desired   = _index_for_key(hm, hm._data[current_index].k);
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
namespace ds {
  namespace hashmap {
    template <typename K, typename V>
    HashMap<K, V> init(mem::Arena* a, U64 capacity) {
      assert((capacity & (capacity - 1)) == 0 && "HashMap capacity must be power of two");

      HashMap<K, V> hm{
          ._size     = 0,
          ._capacity = capacity,
          ._data     = mem::arena::alloc<typename ds::HashMap<K, V>::_KeyValue>(
              a,
              capacity * sizeof(typename ds::HashMap<K, V>::_KeyValue)),
          ._a   = a,
      };

      _fill_empty_values<K, V>(hm);

      return hm;
    }

    template <typename K, typename V>
    void grow(HashMap<K, V>& hm) {
      U64  old_capacity = hm._capacity;
      auto old_data     = hm._data;
      hm._size          = 0;
      hm._capacity *= 2;
      hm._data = mem::arena::alloc<typename ds::HashMap<K, V>::_KeyValue>(
          hm._a,
          hm._capacity * sizeof(typename ds::HashMap<K, V>::_KeyValue));

      assert(hm._data != nullptr);

      _fill_empty_values<K, V>(hm);

      for (U64 i = 0; i < old_capacity; i++) {
        if (old_data[i].k != empty_value<K>()) insert(hm, old_data[i].k, old_data[i].v);
      }
    }

    template <typename K, typename V>
    void clear(HashMap<K, V>& hm) {
      hm._size = 0;
      _fill_empty_values<K, V>(hm);
    }

    template <typename K, typename V>
    V* insert(HashMap<K, V>& hm, const K& k, const V& v) {
      assert(hm._capacity != 0 && "init not called");

      if (hm._size >= hm._capacity) {
        grow(hm);
      }

      U64 hash  = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      U64 dist  = 0;
      hm._size++;
      K k_copy = k;
      V v_copy = v;

      for (;;) {
        if (hm._data[index].k == empty_value<K>()) {
          hm._data[index].k = k_copy;
          hm._data[index].v = v_copy;
          return &hm._data[index].v;
        }

        U64 desired   = _index_for_key(hm, hm._data[index].k);
        U64 curr_dist = (index + hm._capacity - desired) & (hm._capacity - 1);
        if (curr_dist < dist) {
          std::swap(k_copy, hm._data[index].k);
          std::swap(v_copy, hm._data[index].v);

          dist = curr_dist;
        }
        dist++;
        index = (index + 1) & (hm._capacity - 1);
      }
    }

    template <typename K, typename V>
    bool contains(const ::ds::HashMap<K, V>& hm, const K& k) {
      U32 not_used;
      return array_index(hm, k, not_used);
    }

    template <typename K, typename V>
    V* value(const HashMap<K, V>& hm, const K& k) {
      U64 hash  = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      U64 dist  = 0;
      for (;;) {
        if (hm._data[index].k == empty_value<K>()) {
          return nullptr;
        }

        if (hm._data[index].k == k) {
          return &hm._data[index].v;
        }

        U64 desired   = _index_for_key(hm, hm._data[index].k);
        U64 curr_dist = (index + hm._capacity - desired) & (hm._capacity - 1);

        if (curr_dist < dist) {
          return nullptr;
        }

        dist++;
        index = (index + 1) & (hm._capacity - 1);
      }
    }

    template <typename K, typename V>
    void erase(HashMap<K, V>& hm, const K& k) {
      U64 hash  = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      for (;;) {
        if (hm._data[index].k == k) {
          hm._size--;
          _remove(hm, index);
          return;
        }
        index = (index + 1) & (hm._capacity - 1);
      }
    }

  }
}

#endif
