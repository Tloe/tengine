add_library(render)

set(HEADERS
    defines.h
    stb/stb_image.h
    stb/stb_truetype.h
    tiny_obj_loader/tiny_obj_loader.h
    textures.h
    meshes.h
    fonts.h
    ui.h
    render.h
    frame.h)
set(SOURCES
    textures.cpp
    meshes.cpp
    fonts.cpp
    ui.cpp
    render.cpp
    frame.cpp)

find_package(volk REQUIRED)

add_subdirectory(vulkan)

find_package(glm REQUIRED)

target_sources(render PUBLIC ${HEADERS} PRIVATE ${SOURCES})

get_target_property(VOLK_INCLUDE_DIR volk::volk INTERFACE_INCLUDE_DIRECTORIES)

message(STATUS "Volk include dir: ${VOLK_INCLUDE_DIR}")
target_include_directories(render PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${VOLK_INCLUDE_DIR})

target_link_libraries(render PRIVATE core SDL3::SDL3 glm::glm volk::volk)

