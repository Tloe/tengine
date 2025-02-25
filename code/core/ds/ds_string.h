#ifndef STRING_H
#define STRING_H

#include "types.h"

namespace mem {struct Arena; }
namespace mem { typedef Arena* ArenaPtr; }

namespace ds {
  struct String {
    U32 _size = 0;
    char *_data = nullptr;
    mem::ArenaPtr _a;
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
    String init(mem::ArenaPtr a);
    String init(mem::ArenaPtr a, const String &str);
    String init(mem::ArenaPtr a, const char *cstr);
    String init(mem::ArenaPtr a, const char c);

    char *c_str(const String &str);
  }
}

#endif
