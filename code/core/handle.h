#pragma once

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
}

