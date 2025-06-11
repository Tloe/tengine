#pragma once

#include "ds_sparse_array.h"
#include "handles.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>

template <typename T, U32 MaxInstances>
struct ResourceRegistry {
  StaticSparseArray8<StaticSparseArray16<T, MaxInstances, MaxInstances>, 10, 10> arena_storages;
  const char*                                                                    name;
};

namespace {
  template <typename T, U32 MaxInstances>
  StaticSparseArray16<T, MaxInstances, MaxInstances>*
  _storage(ResourceRegistry<T, MaxInstances>& rr, U8 arena_index) {
    if (!sparse::has(rr.arena_storages, arena_index)) {
      sparse::insert(rr.arena_storages,
                     arena_index,
                     sparse::init16<T, MaxInstances, MaxInstances>(ArenaHandle{
                         .value = arena_index,
                     }));
    }

    return sparse::value(rr.arena_storages, arena_index);
  }
}

namespace resource_registry {
  template <typename T, U32 MI>
  ResourceRegistry<T, MI> init(ArenaHandle arena, const char* name) {
    return ResourceRegistry<T, MI>{
        .arena_storages = sparse::init8<StaticSparseArray16<T, MI, MI>, 10, 10>(arena),
        .name           = name,
    };
  }

  template <typename Handle, typename T, U32 MaxInstances>
  Handle insert(ResourceRegistry<T, MaxInstances>& rr, ArenaHandle arena, const T& t) {
    auto storage = _storage(rr, arena.value);

    auto handle = Handle{
        .arena_index = static_cast<I8>(arena.value),
        .value       = sparse::next_id(*storage),
    };

    if (!sparse::insert(*storage, handle.value, t)) {
      printf("failed to insert Resource in '%s' Registry", rr.name);
      exit(0);
    }

    return handle;
  }

  template <typename Handle, typename T, U32 MaxInstances>
  T* value(ResourceRegistry<T, MaxInstances>& rr, Handle handle) {
    if (!sparse::has(rr.arena_storages, U8(handle.arena_index))) {
      return nullptr;
    }

    auto storage = _storage(rr, handle.arena_index);

    return sparse::value(*storage, handle.value);
  }

  template <typename Handle, typename T, U32 MaxInstances>
  void reset_arena_storage(ResourceRegistry<T, MaxInstances>& rr, U8 arena_index) {
    sparse::insert(rr.arena_storages,
                   arena_index,
                   sparse::init16<T, MaxInstances, MaxInstances>(ArenaHandle{
                       .value = arena_index,
                   }));
  }

  template <typename Handle, typename T, U32 MaxInstances>
  void remove(ResourceRegistry<T, MaxInstances>& rr, Handle handle) {
    if (!sparse::has(rr.arena_storages, U8(handle.arena_index))) {
      return;
    }

    auto storage = _storage(rr, handle.arena_index);

    sparse::remove(*storage, handle.value);
  }
}
