set(HEADERS
    sim.h)

set(SOURCES
    main.cpp
    sim.cpp)

target_sources(fightspace PUBLIC ${HEADERS} PRIVATE ${SOURCES})

add_executable(fightspace)

target_link_libraries(fightspace
  PRIVATE
    core
    render
    engine)

set_target_properties(fightspace
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/exec")


