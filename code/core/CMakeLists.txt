add_library(core)

set(HEADERS
    types.h)

target_sources(core PUBLIC ${HEADERS})

target_include_directories(core PUBLIC ds memory ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(ds)
add_subdirectory(memory)
