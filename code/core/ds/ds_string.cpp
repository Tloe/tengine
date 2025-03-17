#include "ds_string.h"

#include "arena.h"

#include <cstring>

String string::init(ArenaHandle arena_handle) {
  String s{
      ._size         = 1,
      ._data         = arena::alloc<char>(arena_handle, 2),
      ._arena_handle = arena_handle,
  };

  s._data[0] = ' ';
  s._data[1] = '\0';

  return s;
}

String string::init(ArenaHandle arena_handle, const char* cstr) {
  auto size = static_cast<U32>(strlen(cstr));

  String s{
      ._size         = size,
      ._data         = arena::alloc<char>(arena_handle, size + 1),
      ._arena_handle = arena_handle,
  };

  strcpy(s._data, cstr);

  return s;
}

String string::init(ArenaHandle arena_handle, const String& str) {
  String s{._size         = str._size,
           ._data         = arena::alloc<char>(arena_handle, str._size + 1),
           ._arena_handle = arena_handle};
  strcpy(s._data, str._data);

  return s;
}

String string::init(ArenaHandle arena_handle, const char c) {
  String s{
      ._size         = 1,
      ._data         = arena::alloc<char>(arena_handle, 2),
      ._arena_handle = arena_handle,
  };
  s._data[0] = c;
  s._data[1] = '\0';

  return s;
}

bool operator==(const String& lhs, const String& rhs) {
  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data == rhs._data;
  }
  return std::strcmp(reinterpret_cast<const char*>(lhs._data),
                     reinterpret_cast<const char*>(rhs._data)) == 0;
}

bool operator==(const String& lhs, const char* rhs) {
  return std::strcmp(reinterpret_cast<const char*>(lhs._data), rhs) == 0;
}

bool operator!=(const String& lhs, const String& rhs) {
  if (lhs._data == nullptr || rhs._data == nullptr) {
    return lhs._data != rhs._data;
  }
  return std::strcmp(reinterpret_cast<const char*>(lhs._data),
                     reinterpret_cast<const char*>(rhs._data)) != 0;
}

bool operator!=(const String& lhs, const char* rhs) {
  return std::strcmp(reinterpret_cast<const char*>(lhs._data), rhs) != 0;
}

String operator+(const String& lhs, const String& rhs) {
  String s{
      ._size         = lhs._size + rhs._size,
      ._data         = arena::alloc<char>(lhs._arena_handle, lhs._size + rhs._size + 1),
      ._arena_handle = lhs._arena_handle,
  };

  memcpy(s._data, lhs._data, lhs._size);
  strcpy(s._data + lhs._size, rhs._data);

  return s;
}

String operator+(const String& lhs, const char* rhs) {
  U32 rhs_size = strlen(rhs);

  String s{
      ._size         = lhs._size + rhs_size,
      ._data         = arena::alloc<char>(lhs._arena_handle, lhs._size + rhs_size + 1),
      ._arena_handle = lhs._arena_handle,
  };

  memcpy(s._data, lhs._data, lhs._size);
  strcpy(s._data + lhs._size, rhs);

  return s;
}

String& operator+=(String& lhs, const String& rhs) {
  char* new_data = arena::alloc<char>(lhs._arena_handle, lhs._size + rhs._size + 1);
  memcpy(new_data, lhs._data, lhs._size);
  strcpy(new_data + lhs._size, rhs._data);
  lhs._data = new_data;
  lhs._size += rhs._size;

  return lhs;
}

String& operator+=(String& lhs, const char* rhs) {
  U32   rhs_size = strlen(rhs);
  char* new_data = arena::alloc<char>(lhs._arena_handle, lhs._size + rhs_size + 1);
  memcpy(new_data, lhs._data, lhs._size);
  strcpy(new_data + lhs._size, rhs);
  lhs._size += rhs_size;

  return lhs;
}
