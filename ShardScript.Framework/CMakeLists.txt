project(ShardScript.Framework)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

file(GLOB_RECURSE FRAMEWORK_SOURCES "*.cpp")
file(GLOB_RECURSE FRAMEWORK_HEADERS "*.h")
file(GLOB_RECURSE FRAMEWORK_RESOURCES "*.rc" "*.ss")

add_library(ShardScript.Framework SHARED 
    ${FRAMEWORK_SOURCES} 
    ${FRAMEWORK_HEADERS}
    ${FRAMEWORK_RESOURCES}
)

target_include_directories(ShardScript.Framework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(ShardScript.Framework PUBLIC
    ShardScript
)

set_target_properties(ShardScript.Framework PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)