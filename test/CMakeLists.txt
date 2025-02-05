find_package(Catch2 3 REQUIRED)


add_executable(tests test_datastructures.cpp test_main.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(tests OUTPUT_DIR "${PROJECT_SOURCE_DIR}/test/tests")

target_link_libraries(tests PRIVATE core)

set_target_properties(tests
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
      "${PROJECT_SOURCE_DIR}/test/exec")
