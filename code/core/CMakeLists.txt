add_library(core)

set(HEADERS
    io.h
    handle.h
    types.h)

set(SOURCES
    io.cpp)

target_sources(core PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(core PUBLIC ds memory ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(ds)
add_subdirectory(memory)
