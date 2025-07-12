#include "simulation.h"

#include "arena.h"
#include "ds_dynamic_sparse_array.h"
#include "ds_sparse_array.h"
#include "types.h"

namespace {
  auto mem_level = arena::by_name("level");

  const U8 CHUNK_WIDTH  = 64;
  const U8 CHUNK_HEIGHT = 64;

  const U8 VIEW_WIDTH  = 192;
  const U8 VIEW_HEIGHT = 108;

  const U8 GHOST_BORDER_WIDTH = 3;

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

  U32 view_x = 0;
  U32 view_y = 0;

  U8* _gpu_memory = nullptr;

  struct Chunk {
    U8 x;
    U8 y;

    BorderChange border_changes = BorderChange::NONE;

    DynamicSparseArray<MaterialType, U16> materials[2];
    DynamicSparseArray<U8, U16>           velocitity_x[2];
    DynamicSparseArray<U8, U16>           velocitity_y[2];
    DynamicSparseArray<U8, U16>           flags[2];
  };

  DynamicSparseArray<Chunk, U16> alive;
  DynamicSparseArray<Chunk, U16> dead;

  U8 current_buffer_index = 0;

  Chunk* _get_chunk(U32 x, U32 y) {
    U32 chunk_x = x / CHUNK_WIDTH;
    U32 chunk_y = y / CHUNK_HEIGHT;

    auto chunk = sparse::value(
        alive,
        static_cast<U16>(chunk_x + chunk_y * (CHUNK_WIDTH / CHUNK_HEIGHT)));

    if (!chunk) {
      sparse::value(
          dead,
          static_cast<U16>(chunk_x + chunk_y * (CHUNK_WIDTH / CHUNK_HEIGHT)));
    }

    assert(chunk != nullptr && "Chunk from absolute x/y "
                               "not found");

    return chunk;
  }

  MaterialType* _cell_material(Chunk* chunk, U32 x, U32 y) {
    if (y < CHUNK_HEIGHT - 1) {
      return sparse::value(chunk->materials[current_buffer_index],
                           static_cast<U16>((y + 1) * CHUNK_WIDTH + x));
    }
    return nullptr;
  }

  Chunk _init_chunk(U32 chunk_x, U32 chunk_y) {
    auto chunk_width_height = CHUNK_WIDTH * CHUNK_HEIGHT;

    Chunk new_chunk{
        .x = static_cast<U8>(chunk_x),
        .y = static_cast<U8>(chunk_y),
    };

    for (U8 i = 0; i < 2; ++i) {
      new_chunk.materials[i] =
          sparse::init<MaterialType, U16>(mem_level,
                                          chunk_width_height,
                                          chunk_width_height);

      for (U32 material_i = 0; material_i < chunk_width_height; ++material_i) {
        new_chunk.materials[i]._data._data[material_i] = MaterialType::AIR;
      }

      new_chunk.velocitity_x[i] = sparse::init<U8, U16>(mem_level,
                                                        chunk_width_height,
                                                        chunk_width_height);

      new_chunk.velocitity_y[i] = sparse::init<U8, U16>(mem_level,
                                                        chunk_width_height,
                                                        chunk_width_height);

      new_chunk.flags[i] = sparse::init<U8, U16>(mem_level,
                                                 chunk_width_height,
                                                 chunk_width_height);
    }

    return new_chunk;
  }

  void _init_gpu_memory() {
    U32 start_chunk_x = view_x / CHUNK_WIDTH;
    U32 start_chunk_y = view_y / CHUNK_HEIGHT;
    U32 num_chunks_x  = (VIEW_WIDTH + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
    U32 num_chunks_y  = (VIEW_HEIGHT + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;

    for (U32 chunk_x = start_chunk_x; chunk_x < start_chunk_x + num_chunks_x;
         ++chunk_x) {
      for (U32 chunk_y = start_chunk_y; chunk_y < start_chunk_y + num_chunks_y;
           ++chunk_y) {
        auto chunk = _get_chunk(chunk_x * CHUNK_WIDTH, chunk_y * CHUNK_HEIGHT);

        U32 gpu_offset =
            (chunk_y * CHUNK_HEIGHT) * (CHUNK_WIDTH * num_chunks_x) +
            (chunk_x * CHUNK_WIDTH);

        memcpy(_gpu_memory + gpu_offset,
               chunk->materials[current_buffer_index]._data._data,
               CHUNK_WIDTH * CHUNK_HEIGHT);
      }
    }
  }

  void _update_gpu_memory() {
    U32 start_chunk_x = view_x / CHUNK_WIDTH;
    U32 start_chunk_y = view_y / CHUNK_HEIGHT;
    U32 num_chunks_x  = (VIEW_WIDTH + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
    U32 num_chunks_y  = (VIEW_HEIGHT + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;

    for (U32 chunk_x = start_chunk_x; chunk_x < start_chunk_x + num_chunks_x;
         ++chunk_x) {
      for (U32 chunk_y = start_chunk_y; chunk_y < start_chunk_y + num_chunks_y;
           ++chunk_y) {
        U16 chunk_index = static_cast<U16>(chunk_x + chunk_y * num_chunks_x);

        if (chunk_index >= alive._size) continue;

        auto chunk = &alive._data._data[chunk_index];

        if (!chunk) continue;

        U32 gpu_offset =
            (chunk_y * CHUNK_HEIGHT) * (CHUNK_WIDTH * num_chunks_x) +
            (chunk_x * CHUNK_WIDTH);

        memcpy(_gpu_memory + gpu_offset,
               chunk->materials[current_buffer_index]._data._data,
               CHUNK_WIDTH * CHUNK_HEIGHT);
      }
    }
  }
}

void simulation::init(
    U32 view_x, U32 view_y, U32 level_width, U32 level_height, U8* gpu_memory) {
  U32 num_chunks_x = (level_width + CHUNK_WIDTH - 1) / CHUNK_WIDTH;
  U32 num_chunks_y = (level_height + CHUNK_HEIGHT - 1) / CHUNK_HEIGHT;

  const auto chunks_n = num_chunks_x * num_chunks_y;

  alive = sparse::init<Chunk, U16>(mem_level, chunks_n, chunks_n);
  dead  = sparse::init<Chunk, U16>(mem_level, chunks_n, chunks_n);

  for (U32 chunk_x = 0; chunk_x < num_chunks_x; ++chunk_x) {
    for (U32 chunk_y = 0; chunk_y < num_chunks_y; ++chunk_y) {
      Chunk new_chunk = _init_chunk(chunk_x, chunk_y);

      sparse::insert(alive,
                     static_cast<U16>(chunk_x + chunk_y * num_chunks_x),
                     new_chunk);
    }
  }

  _gpu_memory = gpu_memory;

  _init_gpu_memory();
}

void simulation::simulate() {
  for (U32 i = 0; i < alive._size; ++i) {
    auto chunk = &alive._data._data[i];

    for (U32 x = 0; x < CHUNK_WIDTH; ++x) {
      for (U32 y = 0; y < CHUNK_HEIGHT; ++y) {
        const U16 index = y * CHUNK_WIDTH + x;
        auto      material =
            sparse::value(chunk->materials[current_buffer_index], index);

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
            printf("material not "
                   "implemented yet\n");
            break;
        }
      }
    }
  }

  current_buffer_index = (current_buffer_index + 1) % 2;

  _update_gpu_memory();
}

void simulation::set_view(U32 x, U32 y) {
  view_x = x;
  view_y = y;
}

void simulation::add_cell(U32 x, U32 y, MaterialType type) {
  auto chunk = _get_chunk(x, y);

  U32 local_x = x % CHUNK_WIDTH;
  U32 local_y = y % CHUNK_HEIGHT;
  chunk->materials[current_buffer_index]
      ._data._data[local_y * CHUNK_WIDTH + local_x] = type;
}
