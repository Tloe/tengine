add_executable(fightspace main.cpp)

target_link_libraries(fightspace
  PUBLIC
    vulkan
  PRIVATE
    core
    render
    engine)

set_target_properties(fightspace
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/exec")


