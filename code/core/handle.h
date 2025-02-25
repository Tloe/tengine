#ifndef HANDLE_H
#define HANDLE_H

// usage
// struct some_tag{};
// typedef Handle<some_tag, U32, -1>

/* struct test_tag; */
/* typedef Handle<test_tag, int, -1> test_handle; */
/* handle::test_handle handle::func() { */
/*   test_handle handle{.value=1}; */
/*   auto t = handle::invalid(handle); */
/*   return handle; */
/* } */

template <typename Tag, typename T, T default_value> struct Handle {
  T value = default_value;
};

template <typename Tag, typename T, T default_value>
bool operator==(const Handle<Tag, T, default_value> &lhs, const Handle<Tag, T, default_value> &rhs){
  return lhs.value == rhs.value;
}


template <typename Tag, typename T, T default_value>
bool operator!=(const Handle<Tag, T, default_value> &lhs, const Handle<Tag, T, default_value> &rhs){
  return lhs.value != rhs.value;
}

namespace handle {
  template <typename Tag, typename T, T default_value>
  bool invalid(Handle<Tag, T, default_value> handle) {
    return handle.value == default_value;
  }
}

#endif
