add_executable(play2 main.cpp)

target_link_libraries(play2
  PRIVATE
    core
    render
    engine)

set_target_properties(play2
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/exec")


