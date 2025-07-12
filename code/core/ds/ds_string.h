#pragma once

#include "ds_array_static.h"
#include "memory/handles.h"
#include "types.h"

struct String {
  U32         _size = 0;
  U32        _capacity = 0;
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
  String init(ArenaHandle arena_handle, const char* chars, U32 size);
  String init(ArenaHandle arena_handle, const char c);

  char* c_str(const String& str);
}

#define S_STRING(str) string::init(arena::scratch(), str)
#define F_STRING(str) string::init(arena::frame(), str)

template <U32 SIZE, U32 MAX_STRING_SIZE>
struct StringArray {
  StaticArray<String, SIZE> strings;
};
