project(ShardScript)

include(cmake/libuv.cmake)
include(cmake/mimalloc.cmake)

file(GLOB_RECURSE SHARDSCRIPT_SOURCES "src/*.cpp")
file(GLOB_RECURSE SHARDSCRIPT_HEADERS "include/*.hpp")

add_library(${PROJECT_NAME} SHARED
    ${SHARDSCRIPT_SOURCES}
    ${SHARDSCRIPT_HEADERS}
    ${LIBUV_SOURCES}
    ${MIMALLOC_SOURCES}
)

set_source_files_properties(${LIBUV_SOURCES} PROPERTIES COMPILE_FLAGS "-w")
set_source_files_properties(${MIMALLOC_SOURCES} PROPERTIES COMPILE_FLAGS "-w")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${LIBUV_DEFINES}
    ${MIMALLOC_DEFINES}
    "SHARD_API_EXPORT"
    "UNICODE"
    "_UNICODE"
    "_CRT_SECURE_NO_WARNINGS"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/mimalloc/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/libuv/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/libuv/src"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBUV_LIBRARIES}
)

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_DL_LIBS}
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--export-all-symbols")
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types -Wno-discarded-qualifiers>)
endif()

set_target_properties(ShardScript PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    DISABLE_PRECOMPILE_HEADERS ON
)

if(ENABLE_ASAN)
    # --- CLANG (Windows, macOS, Linux) ---
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address /Oy-)
        else()
            target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        endif()

        if(WIN32)
            set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
            target_link_options(${PROJECT_NAME} PRIVATE 
                "/wholearchive:clang_rt.asan_dynamic-x86_64.lib"
                "/wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib"
            )
        else()
            target_link_options(${PROJECT_NAME} PRIVATE -shared-libasan)
        endif()

    # --- GCC (Linux) ---
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
        target_link_options(${PROJECT_NAME} PRIVATE -shared-libasan)

    # --- MSVC (Windows Native) ---
    elseif(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    endif()
endif()

