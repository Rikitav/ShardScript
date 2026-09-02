project(ShardScript)

include(CMake/libuv.cmake)
include(CMake/mimalloc.cmake)

file(GLOB_RECURSE SHARDSCRIPT_SOURCES "src/*.cpp")
file(GLOB_RECURSE SHARDSCRIPT_HEADERS "include/*.hpp")
file(GLOB_RECURSE SHARDSCRIPT_OTHER "cpp.hint" "*.md")

add_library(${PROJECT_NAME} SHARED
    ${SHARDSCRIPT_SOURCES}
    ${SHARDSCRIPT_HEADERS}
    ${SHARDSCRIPT_OTHER}
    ${LIBUV_SOURCES}
    ${MIMALLOC_SOURCES})

if(ENABLE_ASAN)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
    target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv/include"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc/include"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE "SHARD_API_EXPORT;UNICODE;_UNICODE")

if(LIBUV_DEFINES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${LIBUV_DEFINES})
endif()

if(MIMALLOC_DEFINES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${MIMALLOC_DEFINES})
endif()

if(LIBUV_LIBRARIES)
    target_link_libraries(ShardScript PRIVATE ${LIBUV_LIBRARIES})
endif()

if(NOT WIN32)
  target_link_libraries(ShardScript PRIVATE ${CMAKE_DL_LIBS})
endif()

# MinGW does not auto-export C++ symbols the way MSVC's /EXPORTALL does.
# Force the linker to export every symbol so Framework libraries can link to helpers like
# shard::SetTaskState, shard::ResumeContinuation, shard::CreateRuntimeException, etc.
if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_link_options(ShardScript PRIVATE "-Wl,--export-all-symbols")
endif()

# GCC/Clang (including MinGW) are stricter than MSVC about some libuv Windows C code.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(ShardScript PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types -Wno-discarded-qualifiers>)
endif()

use_props(ShardScript "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(ShardScript PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TARGET_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TARGET_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TARGET_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${TARGET_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TARGET_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${TARGET_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${TARGET_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TARGET_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${TARGET_OUTPUT_DIR}"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    VS_GLOBAL_KEYWORD "Win32Proj"
)

set(ROOT_NAMESPACE ShardScript)
