#pragma once

#include "ds_bitarray.h"
#include "lifetime.h"

#include <cassert>
#include <cstdio>
#include <utility>


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

namespace handles {
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
      ha.next_hint = handle.value;
    }
  }

  template <typename Handle, typename ValueT, U32 MAX>
  bool is_allocated(Allocator<Handle, ValueT, MAX>& ha, Handle handle) {
    return bitarray::get(ha.values, handle.value);
  }

  inline U16 pack_lifetime(U16 handle_value, LifeTime lifetime) {
    assert(handle_value < (1 << 12));
    assert(std::to_underlying(lifetime) < (1 << 4));
    return (std::to_underlying(lifetime) << 12) | handle_value;
  }

  inline void unpack_lifetime(U16 packed, U16& handle_value, LifeTime& lifetime) {
    handle_value = packed & 0x0FFF;
    lifetime     = static_cast<LifeTime>((packed >> 12) & 0x0F);
  }

  inline U32 pack_lifetime(U32 handle_value, LifeTime lifetime) {
    assert(handle_value < (1u << 28));
    assert(std::to_underlying(lifetime) < (1 << 4));
    return (std::to_underlying(lifetime) << 28) | handle_value;
  }

  inline void unpack_lifetime(U32 packed, U32& handle_value, LifeTime& lifetime) {
    handle_value = packed & 0x0FFFFFFF;
    lifetime     = static_cast<LifeTime>((packed >> 28) & 0x0F);
  }
}
