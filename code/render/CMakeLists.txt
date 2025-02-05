add_library(render)

set(HEADERS
    stb_image/stb_image.h
    tiny_obj_loader/tiny_obj_loader.h
    vulkan/instance.h
    vertex_buffer.h
    render.h)
set(SOURCES
    vulkan/instance.cpp
    vertex_buffer.cpp
    render.cpp)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(render PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(render PRIVATE core SDL3::SDL3)

add_subdirectory(vulkan)
