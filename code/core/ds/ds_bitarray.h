#pragma once

#include "types.h"

template <typename T, U32 SIZE>
struct BitArray {
  static constexpr U32 BitsPerWord = sizeof(T) * 8;
  static constexpr U32 NumWords    = (SIZE + BitsPerWord - 1) / BitsPerWord;

  T data[NumWords];
};

namespace bitarray {
  template <typename T, U32 SIZE>
  BitArray<T, SIZE> init();

  template <typename T, U32 SIZE>
  void clear_all(BitArray<T, SIZE>& ba);

  template <typename T, U32 SIZE>
  void set(BitArray<T, SIZE>& ba, U32 index);

  template <typename T, U32 SIZE>
  void clear(BitArray<T, SIZE>& ba, U32 index);

  template <typename T, U32 SIZE>
  bool get(BitArray<T, SIZE>& ba, U32 index);

  template <typename T, U32 SIZE>
  U32 size(BitArray<T, SIZE>& ba);

  template <typename T, U32 SIZE>
  U32 find_first_zero(BitArray<T, SIZE>& ba);

  template <typename T, U32 SIZE>
  BitArray<T, SIZE> init() {
    BitArray<T, SIZE> ba;
    clear_all(ba);
    return ba;
  }

  template <typename T, U32 SIZE>
  void clear_all(BitArray<T, SIZE>& ba) {
    for (U32 i = 0; i < BitArray<T, SIZE>::NumWords; ++i)
      ba.data[i] = 0;
  }

  template <typename T, U32 SIZE>
  void set(BitArray<T, SIZE>& ba, U32 index) {
    ba.data[index / BitArray<T, SIZE>::BitsPerWord] |=
        (T(1) << (index % BitArray<T, SIZE>::BitsPerWord));
  }

  template <typename T, U32 SIZE>
  void clear(BitArray<T, SIZE>& ba, U32 index) {
    ba.data[index / BitArray<T, SIZE>::BitsPerWord] &=
        ~(T(1) << (index % BitArray<T, SIZE>::BitsPerWord));
  }

  template <typename T, U32 SIZE>
  bool get(BitArray<T, SIZE>& ba, U32 index) {
    return (ba.data[index / BitArray<T, SIZE>::BitsPerWord] >>
            (index % BitArray<T, SIZE>::BitsPerWord)) &
           T(1);
  }

  template <typename T, U32 SIZE>
  U32 size(BitArray<T, SIZE>& ba) {
    return SIZE;
  }

  template <typename T, U32 SIZE>
  U32 find_first_zero(BitArray<T, SIZE>& ba) {
    for (U32 word_idx = 0; word_idx < BitArray<T, SIZE>::NumWords; ++word_idx) {
      T word = ba.data[word_idx];
      if (~word) {
        for (U32 bit = 0; bit < BitArray<T, SIZE>::BitsPerWord; ++bit) {
          if (!(word & (T(1) << bit))) {
            U32 idx = word_idx * BitArray<T, SIZE>::BitsPerWord + bit;
            if (idx < BitArray<T, SIZE>::NumBits) return int(idx);
          }
        }
      }
    }
    return -1;
  }
}
