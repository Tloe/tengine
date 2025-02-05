cmake_minimum_required(VERSION 3.31.4)
project(tengine CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "MSVC Compiling only on windows for now.")
    endif()
    set(COMPILE_OPTS "/std:c++17")
elseif(UNIX)
    set(COMPILE_OPTS
      -Wall -Wno-vla -Wno-long-long -pedantic -std=c++20 -stdlib=libc++ -ferror-limit=0 -maes -mssse3)
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_compile_definitions(${COMPILE_DEFENITIONS})
add_compile_options(${COMPILE_OPTS})

add_subdirectory(code)
add_subdirectory(test)
