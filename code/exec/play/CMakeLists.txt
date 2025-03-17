add_executable(play main.cpp)

target_link_libraries(play
  PUBLIC
    vulkan
  PRIVATE
    core
    render)

set_target_properties(play
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/exec")

