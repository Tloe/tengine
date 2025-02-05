#ifndef STRING_H
#define STRING_H

#include "arena.h"
#include "types.h"

namespace ds {
  struct String {
    U32 _size = 0;
    char *_data = nullptr;
    mem::Arena *_a;
  };

  bool operator==(const String &lhs, const String &rhs);
  bool operator==(const String &lhs, const char *rhs);

  bool operator!=(const String &lhs, const String &rhs);
  bool operator!=(const String &lhs, const char *rhs);

  String operator+(const String &lhs, const String &rhs);
  String operator+(const String &lhs, const char *rhs);

  String &operator+=(String &lhs, const String &rhs);
  String &operator+=(String &lhs, const char *rhs);

  namespace string {
    String init(mem::Arena *a);
    String init(mem::Arena *a, const String &str);
    String init(mem::Arena *a, const char *cstr);
    String init(mem::Arena *a, const char c);

    char *c_str(const String &str);
  }
}

#endif
