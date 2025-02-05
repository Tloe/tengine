#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"
#include "ds_string.h"
#include "fnv-1a/fnv.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <types.h>
#include <utility>

// https://thenumb.at/Hashtables/#benchmark-data - based on robin_hood_with_deletion algorithm

namespace ds {
  template <typename K, typename V> struct HashMap {
    U64 _size = 0;
    U64 _capacity = 0;

    struct _KeyValue {
      K k;
      V v;
    };
    _KeyValue *_data;
    mem::Arena *_a;
  };

  namespace hashmap {
    template <typename K, typename V>
    HashMap<K, V> init(mem::Arena *a, U64 size = 0, U64 capacity = 8);
    template <typename K, typename V> void grow(HashMap<K, V> &hm);
    template <typename K, typename V> HashMap<K, V> clear(HashMap<K, V> &hm);
    template <typename K, typename V> void insert(HashMap<K, V> &hm, const K &k, const V &v);
    template <typename K, typename V> bool contains(const HashMap<K, V> &hm, const K &k);
    template <typename K, typename V> V *value(const HashMap<K, V> &hm, const K &k);
    template <typename K, typename V> void erase(HashMap<K, V> &hm, const K &k);
  }
}

namespace {
  inline constexpr U64 EMPTY = UINT64_MAX;

  inline U64 _hash_key(ds::String s) { return fnv_64a_buf(s._data, s._size, FNV1A_64_INIT); }

  inline U64 _hash_key(U64 ui) {
    return fnv_64a_buf(reinterpret_cast<void *>(&ui), sizeof(ui), FNV1A_64_INIT);
  }

  template <typename K, typename V> U64 _index_for_key(const ds::HashMap<K, V> &hm, const K &k) {
    U64 hash = _hash_key(k);
    U64 index = hash & (hm._capacity - 1);
    return index;
  }

  template <typename K, typename V> void _remove(ds::HashMap<K, V> &hm, U64 index) {
    for (;;) {
      hm._data[index].k = EMPTY;
      uint64_t next = (index + 1) & (hm._capacity - 1);
      if (hm._data[next].k == EMPTY)
        return;
      uint64_t desired = _index_for_key(hm, hm._data[next].k);
      if (next == desired)
        return;
      hm._data[index].k = hm._data[next].k;
      hm._data[index].v = hm._data[next].v;
      index = next;
    }
  }

}

namespace ds {
  namespace hashmap {
    template <typename K, typename V> HashMap<K, V> init(mem::Arena *a, U64 size, U64 capacity) {
      assert((capacity & (capacity - 1)) == 0 && "HashMap capacity must be power of two");

      HashMap<K, V> hm{
          ._size = size,
          ._capacity = capacity,
          ._data = mem::arena::alloc<typename ds::HashMap<K, V>::_KeyValue>(
              a,
              capacity * sizeof(typename ds::HashMap<K, V>::_KeyValue)),
          ._a = a,
      };

      std::memset(hm._data, 0xff, sizeof(typename HashMap<K, V>::_KeyValue) * hm._capacity);

      return hm;
    }

    template <typename K, typename V> void grow(HashMap<K, V> &hm) {
      U64 old_capacity = hm._capacity;
      auto old_data = hm._data;
      hm._size = 0;
      hm._capacity *= 2;
      hm._data = mem::arena::alloc<typename ds::HashMap<K, V>::_KeyValue>(hm._a, hm._capacity);

      std::memset(hm._data, 0xff, sizeof(typename HashMap<K, V>::_KeyValue) * hm._capacity);

      for (U64 i = 0; i < old_capacity; i++) {
        if (old_data[i].k < EMPTY)
          insert(hm, old_data[i].k, old_data[i].v);
      }
    }

    template <typename K, typename V> ds::HashMap<K, V> clear(HashMap<K, V> &hm) {
      hm._size = 0;
      std::memset(hm._data, 0xff, sizeof(typename HashMap<K, V>::_KeyValue) * hm._capacity);
    }

    template <typename K, typename V> void insert(HashMap<K, V> &hm, const K &k, const V &v) {
      if (hm._size >= hm._capacity) {
        grow(hm);
      }

      U64 hash = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      U64 dist = 0;
      hm._size++;
      K k_copy = k;
      V v_copy = v;

      for (;;) {
        if (hm._data[index].k == EMPTY) {
          hm._data[index].k = k;
          hm._data[index].v = v;
          return;
        }
        U64 desired = _index_for_key(hm, hm._data[index].k);
        U64 cur_dist = (index + hm._capacity - desired) & (hm._capacity - 1);
        if (cur_dist < dist) {
          std::swap(k_copy, hm._data[index].k);
          std::swap(v_copy, hm._data[index].v);
          dist = cur_dist;
        }
        dist++;
        index = (index + 1) & (hm._capacity - 1);
      }
    }

    template <typename K, typename V> bool contains(const HashMap<K, V> &hm, const K &k) {
      U64 hash = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      U64 dist = 0;
      for (;;) {
        if (hm._data[index].k == EMPTY)
          return false;
        if (hm._data[index].k == k)
          return true;
        U64 desired = _index_for_key(hm, hm._data[index].k);
        U64 cur_dist = (index + hm._capacity - desired) & (hm._capacity - 1);
        if (cur_dist < dist)
          return false;
        dist++;
        index = (index + 1) & (hm._capacity - 1);
      }
    }

    template <typename K, typename V> V *value(const HashMap<K, V> &hm, const K &k) {
      U64 hash = _hash_key(k);
      U64 index = hash & (hm._capacity - 1);
      U64 dist = 0;
      for (;;) {
        if (hm._data[index].k == EMPTY) {
          return nullptr;
        }
        if (hm._data[index].k == k) {
          return &hm._data[index].v;
        }

        U64 desired = _index_for_key(hm, hm._data[index].k);
        U64 cur_dist = (index + hm._capacity - desired) & (hm._capacity - 1);

        if (cur_dist < dist) {
          return nullptr;
        }

        dist++;
        index = (index + 1) & (hm._capacity - 1);
      }
    }

    template <typename K, typename V> void erase(HashMap<K, V> &hm, const K &k) {
      U64 hash = _hash_key(k);
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

  } // namespace hashmap
} // namespace ds
#endif
