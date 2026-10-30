cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ShardScript VERSION 0.7.0 LANGUAGES C CXX)

include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(MSVC)
    add_compile_options(/EHsc /W4)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- x86-64 Release tuning -------------------------------------------------
# ThinLTO is the single largest interpreter win (cross-TU inlining of the
# bytecode dispatch helpers); AVX2 is safe on every Windows 11 x64 machine.
option(SHARD_ENABLE_AVX2 "Compile x86-64 Release builds for AVX2" ON)
option(SHARD_ENABLE_LTO "Use link-time optimization in Release builds" ON)
set(SHARD_PGO_PROFILE "" CACHE FILEPATH "LLVM profile (.profdata) used to optimize Release builds")

if(SHARD_PGO_PROFILE)
    if(NOT IS_ABSOLUTE "${SHARD_PGO_PROFILE}")
        set(SHARD_PGO_PROFILE "${CMAKE_SOURCE_DIR}/${SHARD_PGO_PROFILE}")
    endif()
    if(NOT EXISTS "${SHARD_PGO_PROFILE}")
        message(FATAL_ERROR "SHARD_PGO_PROFILE does not exist: ${SHARD_PGO_PROFILE}")
    endif()
endif()

if(MSVC)
    if(SHARD_ENABLE_AVX2)
        add_compile_options(
            "$<$<CONFIG:Release>:/arch:AVX2>"
            "$<$<CONFIG:RelWithDebInfo>:/arch:AVX2>")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang-cl: link.exe cannot consume LLVM bitcode, so ThinLTO needs lld
        if(SHARD_ENABLE_LTO)
            add_compile_options(
                "$<$<CONFIG:Release>:-flto=thin>"
                "$<$<CONFIG:RelWithDebInfo>:-flto=thin>")
            add_link_options(
                "$<$<CONFIG:Release>:-fuse-ld=lld>"
                "$<$<CONFIG:RelWithDebInfo>:-fuse-ld=lld>")
        endif()

        if(SHARD_PGO_PROFILE)
            add_compile_options(
                "$<$<CONFIG:Release>:-fprofile-use=${SHARD_PGO_PROFILE}>"
                "$<$<CONFIG:Release>:-Wno-profile-instr-unprofiled>"
                "$<$<CONFIG:Release>:-Wno-profile-instr-out-of-date>"
                # ThinLTO reports stale profile hashes through the backend
                # diagnostic group, both while compiling and at link time
                "$<$<CONFIG:Release>:-Wno-backend-plugin>")
            add_link_options(
                "$<$<CONFIG:Release>:-Wno-backend-plugin>"
                "$<$<CONFIG:RelWithDebInfo>:-Wno-backend-plugin>")
        endif()
    elseif(SHARD_ENABLE_LTO)
        add_compile_options(
            "$<$<CONFIG:Release>:/GL>"
            "$<$<CONFIG:RelWithDebInfo>:/GL>")
        add_link_options(
            "$<$<CONFIG:Release>:/LTCG>"
            "$<$<CONFIG:RelWithDebInfo>:/LTCG>")
    endif()
endif()

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(ShardScript)
#add_subdirectory(ShardScript.Framework)
add_subdirectory(ShardScript.Interpreter)
