#pragma once

#include <cfloat>
#include <cstdint>

using I8            = int8_t;
constexpr I8 I8_MAX = INT8_MAX;

using I16             = int16_t;
constexpr I16 I16_MAX = INT16_MAX;

using I32             = int32_t;
constexpr I32 I32_MAX = INT32_MAX;

using I64             = int64_t;
constexpr I64 I64_MAX = INT64_MAX;

using U8            = uint8_t;
constexpr U8 U8_MAX = UINT8_MAX;

using U16             = uint16_t;
constexpr U16 U16_MAX = UINT16_MAX;

using U32             = uint32_t;
constexpr U32 U32_MAX = UINT32_MAX;

using U64             = uint64_t;
constexpr U64 U64_MAX = UINT64_MAX;

using F32             = float;
constexpr F32 F32_MAX = FLT_MAX;

using F64             = double;
constexpr F64 F64_MAX = DBL_MAX;

namespace core {
  template <typename T>
  T max_type();

  template <>
  inline I8 max_type() {
    return I8_MAX;
  }

  template <>
  inline I16 max_type() {
    return I16_MAX;
  }

  template <>
  inline I32 max_type() {
    return I32_MAX;
  }
  template <>
  inline I64 max_type() {
    return I64_MAX;
  }

  template <>
  inline U8 max_type() {
    return U8_MAX;
  }

  template <>
  inline U16 max_type() {
    return U16_MAX;
  }

  template <>
  inline U32 max_type() {
    return U32_MAX;
  }
  template <>
  inline U64 max_type() {
    return U64_MAX;
  }
}

