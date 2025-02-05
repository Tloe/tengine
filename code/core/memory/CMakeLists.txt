set(HEADERS
    arena.h)
set(SOURCES
    arena.cpp)

target_sources(core PUBLIC ${HEADERS} PRIVATE ${SOURCES} CMakeLists.txt)

