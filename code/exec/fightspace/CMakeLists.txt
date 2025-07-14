set(HEADERS
    ui.h
    simulation.h)

set(SOURCES
    main.cpp
    ui.cpp
    simulation.cpp)

add_executable(fightspace)

target_sources(fightspace PUBLIC ${HEADERS} PRIVATE ${SOURCES})


target_link_libraries(fightspace
  PRIVATE
    core
    render
    engine)

set_target_properties(fightspace
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/exec")


