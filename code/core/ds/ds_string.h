#pragma once

#include "memory/handles.h"
#include "types.h"

struct String {
  U32         _size = 0;
  char*       _data = nullptr;
  ArenaHandle _arena_handle;
};

bool operator==(const String& lhs, const String& rhs);
bool operator==(const String& lhs, const char* rhs);

bool operator!=(const String& lhs, const String& rhs);
bool operator!=(const String& lhs, const char* rhs);

String operator+(const String& lhs, const String& rhs);
String operator+(const String& lhs, const char* rhs);

String& operator+=(String& lhs, const String& rhs);
String& operator+=(String& lhs, const char* rhs);

namespace string {
  String init(ArenaHandle arena_handle);
  String init(ArenaHandle arena_handle, const String& str);
  String init(ArenaHandle arena_handle, const char* cstr);
  String init(ArenaHandle arena_handle, const char c);

  char* c_str(const String& str);
}
