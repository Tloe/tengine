add_subdirectory(core)
add_subdirectory(exec)
add_subdirectory(render)
add_subdirectory(ui)

add_library(engine)

set(HEADERS
    engine.h)
set(SOURCES
    engine.cpp)

target_sources(engine PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(engine PUBLIC  ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(engine PRIVATE ui core render)

