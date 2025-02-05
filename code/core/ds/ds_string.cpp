#include "ds_string.h"
#include "arena.h"
#include <algorithm>
#include <cstring>

ds::String ds::string::init(mem::Arena * a) {
  ds::String s{._size = 1, ._data = mem::arena::alloc<char>(a, 2), ._a = a};
  s._data[0] = ' ';
  s._data[1] = '\n';

  return s;
}

ds::String ds::string::init(mem::Arena * a, const char *cstr) {
  auto size = static_cast<U32>(strlen(cstr));
  ds::String s{._size = size, ._data = mem::arena::alloc<char>(a, size + 1), ._a = a};
  strcpy(s._data, cstr);

  return s;
}

ds::String ds::string::init(mem::Arena * a, const String &str) {
  String s{._size = str._size, ._data = mem::arena::alloc<char>(a, str._size + 1), ._a = a};
  strcpy(s._data, str._data);

  return s;
}

ds::String ds::string::init(mem::Arena * a, const char c) {
  ds::String s{._size = 1, ._data = mem::arena::alloc<char>(a, 2), ._a = a};
  s._data[0] = c;
  s._data[1] = '\n';

  return s;
}

bool ds::operator==(const ds::String &lhs, const ds::String &rhs) {
  return std::strcmp(reinterpret_cast<const char *>(lhs._data),
                     reinterpret_cast<const char *>(rhs._data)) == 0;
}

bool ds::operator==(const ds::String &lhs, const char *rhs) {
  return std::strcmp(reinterpret_cast<const char *>(lhs._data), rhs) == 0;
}

bool ds::operator!=(const ds::String &lhs, const ds::String &rhs) {
  return std::strcmp(reinterpret_cast<const char *>(lhs._data),
                     reinterpret_cast<const char *>(rhs._data)) != 0;
}

bool ds::operator!=(const ds::String &lhs, const char *rhs) {
  return std::strcmp(reinterpret_cast<const char *>(lhs._data), rhs) != 0;
}

ds::String ds::operator+(const ds::String &lhs, const ds::String &rhs) {
  String s{
      ._size = lhs._size + rhs._size,
      ._data = mem::arena::alloc<char>(lhs._a, lhs._size + rhs._size + 1),
      ._a = lhs._a,
  };

  memcpy(s._data, lhs._data, lhs._size);
  strcpy(s._data + lhs._size, rhs._data);

  return s;
}

ds::String ds::operator+(const ds::String &lhs, const char *rhs) {
  U32 rhs_size = strlen(rhs);

  String s{
      ._size = lhs._size + rhs_size,
      ._data = mem::arena::alloc<char>(lhs._a, lhs._size + rhs_size + 1),
      ._a = lhs._a,
  };

  memcpy(s._data, lhs._data, lhs._size);
  strcpy(s._data + lhs._size, rhs);

  return s;
}

ds::String &ds::operator+=(ds::String &lhs, const ds::String &rhs) {
  mem::arena::alloc(lhs._a, lhs._size + rhs._size + 1);
  memcpy(lhs._data, lhs._data, lhs._size);
  strcpy(lhs._data + lhs._size, rhs._data);
  lhs._size += rhs._size;

  return lhs;
}

ds::String &ds::operator+=(ds::String &lhs, const char *rhs) {
  U32 rhs_size = strlen(rhs);
  mem::arena::alloc(lhs._a, lhs._size + rhs_size + 1);
  memcpy(lhs._data, lhs._data, lhs._size);
  strcpy(lhs._data + lhs._size, rhs);
  lhs._size += rhs_size;

  return lhs;
}
