add_library(render)

set(HEADERS
    stb/stb_image.h
    stb/stb_truetype.h
    tiny_obj_loader/tiny_obj_loader.h
    textures.h
    meshes.h
    fonts.h
    ui.h
    render.h)
set(SOURCES
    textures.cpp
    meshes.cpp
    fonts.cpp
    ui.cpp
    render.cpp)

find_package(glm REQUIRED)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})

target_include_directories(render PUBLIC  ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(render PRIVATE core SDL3::SDL3 glm::glm)

add_subdirectory(vulkan)
