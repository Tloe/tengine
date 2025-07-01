cmake_minimum_required(VERSION 3.30.3)
project(tengine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "MSVC Compiling only on windows for now.")
    endif()
elseif(UNIX)
    # set(CMAKE_CXX_FLAGS "-Wall -Wno-vla -Wno-long-long -Wno-gnu-statement-expression -pedantic -stdlib=libc++ -ferror-limit=0 -maes -mssse3 -fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_CXX_FLAGS "-Wall -Wno-vla -Wno-long-long -Wno-gnu-statement-expression -pedantic -stdlib=libc++ -ferror-limit=0 -maes -mssse3 -fsanitize=address -fno-omit-frame-pointer -g")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_compile_definitions(${COMPILE_DEFENITIONS})
# add_compile_options(${COMPILE_OPTS})

add_subdirectory(code)
add_subdirectory(test)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()
