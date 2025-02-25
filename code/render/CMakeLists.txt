add_library(render)

set(HEADERS
    stb_image/stb_image.h
    tiny_obj_loader/tiny_obj_loader.h
    vulkan/context.h
    vertex_buffer.h
    render.h)
set(SOURCES
    vulkan/context.cpp
    vertex_buffer.cpp
    render.cpp)

find_package(glm REQUIRED)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(render PUBLIC  ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(render PRIVATE core SDL3::SDL3 glm::glm)

add_subdirectory(vulkan)
