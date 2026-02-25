project(ShardScript)

file(GLOB_RECURSE SHARDSCRIPT_SOURCES "src/*.cpp")
file(GLOB_RECURSE SHARDSCRIPT_HEADERS "include/*.hpp")
file(GLOB_RECURSE SHARDSCRIPT_OTHER "cpp.hint" "*.md")

add_library(${PROJECT_NAME} SHARED 
    ${SHARDSCRIPT_SOURCES} 
    ${SHARDSCRIPT_HEADERS}
    ${SHARDSCRIPT_OTHER}
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "SHARD_API_EXPORT;UNICODE;_UNICODE")

set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_GLOBAL_KEYWORD "Win32Proj")

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE ShardScript)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/single_header.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating single header library..."
)