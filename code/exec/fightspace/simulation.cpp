#include "simulation.h"

#include "arena.h"
#include "ds_dynamic_sparse_array.h"
#include "ds_sparse_array.h"
#include "types.h"

namespace {
  auto mem_level = arena::by_name("level");

  const U8  CHUNK_WIDTH        = 64;
  const U8  CHUNK_HEIGHT       = 64;
  const U16 CHUNK_WIDTH_HEIGHT = CHUNK_WIDTH * CHUNK_HEIGHT;

  const U8 VIEW_WIDTH  = 192;
  const U8 VIEW_HEIGHT = 108;

  const U8 GHOST_BORDER_WIDTH = 3;

  const U16 MAX_CHUNKS = U16_MAX;

  enum class BorderChange : U8 {
    NONE         = 0,
    TOP          = 1 << 0,
    BOTTOM       = 1 << 1,
    LEFT         = 1 << 2,
    RIGHT        = 1 << 3,
    TOP_LEFT     = 1 << 4,
    TOP_RIGHT    = 1 << 5,
    BOTTOM_LEFT  = 1 << 6,
    BOTTOM_RIGHT = 1 << 7,
  };

  U32 _view_x         = 0;
  U32 _view_y         = 0;
  U32 _chunks_x_count = 0;
  U32 _chunks_y_count = 0;
  U32 _chunks_count   = 0;

  U8* _gpu_memory = nullptr;

  struct Chunk {
    U8 x;
    U8 y;

    BorderChange border_changes = BorderChange::NONE;

    StaticArray<MaterialType, CHUNK_WIDTH * CHUNK_HEIGHT> materials[2];
    StaticArray<U8, CHUNK_WIDTH * CHUNK_HEIGHT>           velocity_x[2];
    StaticArray<U8, CHUNK_WIDTH * CHUNK_HEIGHT>           velocity_y[2];
    StaticArray<U8, CHUNK_WIDTH * CHUNK_HEIGHT>           flags[2];
  };

  StaticSparseArray16<Chunk, MAX_CHUNKS, MAX_CHUNKS> alive;
  StaticSparseArray16<Chunk, MAX_CHUNKS, MAX_CHUNKS> dead;

  U8 current_buffer_index = 0;

  Chunk* _get_chunk(U32 chunk_x, U32 chunk_y) {
    assert(chunk_x < _chunks_x_count && chunk_y < _chunks_y_count &&
           "Chunk out of bounds");

    U16 chunk_index = chunk_x + chunk_y * _chunks_x_count;

    auto chunk = sparse::value(alive, chunk_index);

    if (!chunk) {
      chunk = sparse::value(dead, chunk_index);
    }

    assert(chunk != nullptr && "Chunk not found");

    return chunk;
  }

  Chunk* _get_chunk_global(U32 x, U32 y) {
    U32 chunk_x = x / CHUNK_WIDTH;
    U32 chunk_y = y / CHUNK_HEIGHT;

    return _get_chunk(chunk_x, chunk_y);
  }

  MaterialType* _cell_material(Chunk* chunk, U32 x, U32 y) {
    if (y < CHUNK_HEIGHT - 1) {
      return &chunk->materials[current_buffer_index]
                  .data[(y + 1) * CHUNK_WIDTH + x];
    }
    return nullptr;
  }

  Chunk _init_chunk(U8 chunk_x, U8 chunk_y) {
    Chunk new_chunk{.x = chunk_x, .y = chunk_y};

    for (U8 buffer_i = 0; buffer_i < 2; ++buffer_i) {
      new_chunk.materials[buffer_i] =
          array::init<MaterialType, CHUNK_WIDTH_HEIGHT>(mem_level,
                                                        MaterialType::AIR);

      new_chunk.velocity_x[buffer_i] =
          array::init<U8, CHUNK_WIDTH_HEIGHT>(mem_level);
      new_chunk.velocity_y[buffer_i] =
          array::init<U8, CHUNK_WIDTH_HEIGHT>(mem_level);

      new_chunk.flags[buffer_i] =
          array::init<U8, CHUNK_WIDTH_HEIGHT>(mem_level);
    }

    printf("Initialized chunk at %d, %d\n", new_chunk.x, new_chunk.y);

    return new_chunk;
  }

  void _init_gpu_memory() {
    U32 start_chunk_x = _view_x / CHUNK_WIDTH;
    U32 start_chunk_y = _view_y / CHUNK_HEIGHT;
    U32 end_x         = _view_x + VIEW_WIDTH - 1;
    U32 end_y         = _view_y + VIEW_HEIGHT - 1;
    U32 num_chunks_x  = (end_x / CHUNK_WIDTH) - start_chunk_x + 1;
    U32 num_chunks_y  = (end_y / CHUNK_HEIGHT) - start_chunk_y + 1;

    printf("%d %d chunks in view: %u x %u\n",
           _view_x,
           _view_y,
           num_chunks_x,
           num_chunks_y);

    for (U32 chunk_y = start_chunk_y; chunk_y < start_chunk_y + num_chunks_y;
         ++chunk_y) {
      for (U32 chunk_x = start_chunk_x; chunk_x < start_chunk_x + num_chunks_x;
           ++chunk_x) {

        printf("getting chunk %d %d\n", chunk_x, chunk_y);
        auto chunk = _get_chunk(chunk_x, chunk_y);

        printf("got chunk %d %d\n", chunk->x, chunk->y);

        U32 gpu_offset =
            (chunk_y * CHUNK_HEIGHT) * (CHUNK_WIDTH * num_chunks_x) +
            (chunk_x * CHUNK_WIDTH);

        memcpy(_gpu_memory + gpu_offset,
               chunk->materials[current_buffer_index].data,
               CHUNK_WIDTH * CHUNK_HEIGHT);
      }
    }

    printf("GPU memory initialized\n");
  }

  void _update_gpu_memory() {
    U32 start_chunk_x = _view_x / CHUNK_WIDTH;
    U32 start_chunk_y = _view_y / CHUNK_HEIGHT;
    U32 num_chunks_x  = (VIEW_WIDTH + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
    U32 num_chunks_y  = (VIEW_HEIGHT + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;

    for (U32 chunk_y = start_chunk_y; chunk_y < start_chunk_y + num_chunks_y;
         ++chunk_y) {
      for (U32 chunk_x = start_chunk_x; chunk_x < start_chunk_x + num_chunks_x;
           ++chunk_x) {
        U16 chunk_index = chunk_x + chunk_y * num_chunks_x;

        if (chunk_index >= alive._size) continue;

        auto chunk = sparse::value(alive, chunk_index);

        if (!chunk) continue;

        U32 gpu_offset =
            (chunk_y * CHUNK_HEIGHT) * (CHUNK_WIDTH * num_chunks_x) +
            (chunk_x * CHUNK_WIDTH);

        memcpy(_gpu_memory + gpu_offset,
               chunk->materials[current_buffer_index].data,
               CHUNK_WIDTH * CHUNK_HEIGHT);
      }
    }
  }
}

void print_sim() {
  U32 per_line_count = 0;
  U32 line_count     = 0;

  for (U32 i = 0; i < 192 * 108; ++i) {
    printf("%c",
           _gpu_memory[i] == static_cast<U8>(MaterialType::AIR) ? '.' : '#');
    if (++per_line_count == 192) {
      printf(" - %d\n", line_count++);
      per_line_count = 0;
    }
  }
}

void simulation::init(
    U32 view_x, U32 view_y, U32 level_width, U32 level_height, U8* gpu_memory) {
  assert(level_width % CHUNK_WIDTH == 0 &&
         "Level width must be a multiple of CHUNK_WIDTH");
  assert(level_height % CHUNK_HEIGHT == 0 &&
         "Level height must be a multiple of CHUNK_HEIGHT");

  _view_x = view_x;
  _view_y = view_y;

  _chunks_x_count = level_width / CHUNK_WIDTH;
  _chunks_y_count = level_height / CHUNK_HEIGHT;

  _chunks_count = _chunks_x_count * _chunks_y_count;

  alive = sparse::init16<Chunk, MAX_CHUNKS, MAX_CHUNKS>(mem_level);
  dead  = sparse::init16<Chunk, MAX_CHUNKS, MAX_CHUNKS>(mem_level);

  for (U32 chunk_y = 0; chunk_y < _chunks_y_count; ++chunk_y) {
    for (U32 chunk_x = 0; chunk_x < _chunks_x_count; ++chunk_x) {
      Chunk new_chunk = _init_chunk(chunk_x, chunk_y);

      sparse::insert(alive,
                     static_cast<U16>(chunk_x + chunk_y * _chunks_x_count),
                     new_chunk);
    }
  }

  printf("after insert:\n");
  for (U32 i = 0; i < _chunks_count; ++i) {
    auto chunk = sparse::value(alive, static_cast<U16>(i));
    if (!chunk) continue;

    printf("Chunk %d: %d, %d at %p\n", i, chunk->x, chunk->y, (void*)chunk);
  }

  _gpu_memory = gpu_memory;
  _init_gpu_memory();

  print_sim();
}

void simulation::simulate() {
  for (U32 i = 0; i < alive._size; ++i) {
    auto chunk = &alive._data.data[i];

    for (U32 y = 0; y < CHUNK_HEIGHT; ++y) {
      for (U32 x = 0; x < CHUNK_WIDTH; ++x) {
        const U16 index = y * CHUNK_WIDTH + x;

        auto material = &chunk->materials[current_buffer_index].data[index];

        switch (*material) {
          case MaterialType::SAND: {
            auto down_one_material = _cell_material(chunk, x, y + 1);
            if (y < CHUNK_HEIGHT - 1 &&
                *down_one_material == MaterialType::AIR) {
              *down_one_material = *material;
              *material          = MaterialType::AIR;
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }

  current_buffer_index = (current_buffer_index + 1) % 2;

  // _update_gpu_memory();

  // print_sim();
  printf("-------\n");
}

void simulation::set_view(U32 x, U32 y) {
  _view_x = x;
  _view_y = y;
}

void simulation::add_cell(U32 x, U32 y, MaterialType type) {
  auto chunk = _get_chunk_global(x, y);

  U32 local_x = x % CHUNK_WIDTH;
  U32 local_y = y % CHUNK_HEIGHT;

  chunk->materials[current_buffer_index].data[local_y * CHUNK_WIDTH + local_x] =
      type;
}
