set(HEADERS
    fnv-1a/fnv.h
    fnv-1a/longlong.h
    ds_array_dynamic.h
    ds_array_static.h
    ds_hashmap.h
    ds_string.h)
set(SOURCES
    fnv-1a/hash_64a.cpp
    ds_array_dynamic.cpp
    ds_array_static.cpp
    ds_string.cpp)

target_sources(core PUBLIC ${HEADERS} PRIVATE ${SOURCES} CMakeLists.txt)

