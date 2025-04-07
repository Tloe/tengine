#pragma once

#include "ds_bitarray.h"

#include <cassert>
template <typename Tag, typename T, T default_value>
struct Handle {
  T value = default_value;
};

template <typename Tag, typename T, T default_value>
bool
operator==(const Handle<Tag, T, default_value>& lhs, const Handle<Tag, T, default_value>& rhs) {
  return lhs.value == rhs.value;
}

template <typename Tag, typename T, T default_value>
bool
operator!=(const Handle<Tag, T, default_value>& lhs, const Handle<Tag, T, default_value>& rhs) {
  return lhs.value != rhs.value;
}

namespace handle {
  template <typename Tag, typename T, T default_value>
  bool invalid(Handle<Tag, T, default_value> handle) {
    return handle.value == default_value;
  }

  template <typename Handle, typename ValueT, U32 MAX>
  struct Allocator {
    BitArray<ValueT, MAX> values    = bitarray::init<ValueT, MAX>();
    U32                   next_hint = 0;
  };

  template <typename Handle, typename ValueT, U32 MAX>
  Handle next(Allocator<Handle, ValueT, MAX>& ha) {
    U32 limit = MAX;
    for (U32 i = 0; i < limit; ++i) {
      ValueT value = (ha.next_hint + i) % limit;
      if (!bitarray::get(ha.values, value)) {
        bitarray::set(ha.values, value);
        ha.next_hint = (value + 1) % limit;
        return Handle{.value = value};
      }
    }

    assert(0 && "no more handles available");

    return Handle{};
  }

  template <typename Handle, typename ValueT, U32 MAX>
  void free(Allocator<Handle, ValueT, MAX>& ha, Handle handle) {
    if (handle.value >= MAX) return;

    bitarray::clear(ha.values, handle.value);

    if (handle.value < ha.next_hint) {
      ha.next_hint = handle;
    }
  }

  template <typename Handle, typename ValueT, U32 MAX>
  bool is_allocated(Allocator<Handle, ValueT, MAX>& ha, Handle handle) {
    return bitarray::get(ha.values, handle.value);
  }
}
