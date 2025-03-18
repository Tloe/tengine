add_library(ui)

set(HEADERS
    ui.h
    clay/clay.h)
set(SOURCES
    ui.cpp
    renderer_SDL3.cpp)

target_sources(ui PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(ui PUBLIC  ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(ui PRIVATE core)

