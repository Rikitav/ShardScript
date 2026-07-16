project(ShardScript)

include(CMake/libuv.cmake)

file(GLOB_RECURSE SHARDSCRIPT_SOURCES "src/*.cpp")
file(GLOB_RECURSE SHARDSCRIPT_HEADERS "include/*.hpp")
file(GLOB_RECURSE SHARDSCRIPT_OTHER "cpp.hint" "*.md")

add_library(${PROJECT_NAME} SHARED
    ${SHARDSCRIPT_SOURCES}
    ${SHARDSCRIPT_HEADERS}
    ${SHARDSCRIPT_OTHER}
    ${LIBUV_SOURCES}
)

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
)

target_compile_definitions(${PROJECT_NAME} PRIVATE "SHARD_API_EXPORT;UNICODE;_UNICODE")

if(LIBUV_DEFINES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${LIBUV_DEFINES})
endif()

if(LIBUV_LIBRARIES)
    target_link_libraries(ShardScript PRIVATE ${LIBUV_LIBRARIES})
endif()

if(NOT WIN32)
  target_link_libraries(ShardScript PRIVATE ${CMAKE_DL_LIBS})
endif()

use_props(ShardScript "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(ShardScript PROPERTIES
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
