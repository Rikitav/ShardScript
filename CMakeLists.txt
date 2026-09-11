cmake_minimum_required(VERSION 3.31.6)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ShardScript VERSION 0.7.0 LANGUAGES C CXX)

include(CMake/GlobalSettingsInclude.cmake OPTIONAL)
include(CMake/CompilerId.cmake)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_ASAN ON CACHE BOOL "Enable AddressSanitizer" FORCE)
endif()

if(MSVC)
    add_compile_options(/EHsc /W4)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(ShardScript)
#add_subdirectory(ShardScript.Framework)
#add_subdirectory(ShardScript.Interpreter)
