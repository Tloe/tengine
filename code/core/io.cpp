#include "handles.h"
#include <arena.h>
#include <fstream>
#include <io.h>

DynamicArray<U8> read_file(ArenaHandle arena_handle, const char* filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    printf("failed to open file: '%s'", filename);
    exit(0);
  }

  size_t file_size = (size_t)file.tellg();

  auto buffer = array::init<U8>(arena_handle, file_size, file_size);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer._data), file_size);

  file.close();

  return buffer;
}
