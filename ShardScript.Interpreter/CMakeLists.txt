project(ShardScript.Interpreter)

file(GLOB_RECURSE INTERPRETER_SOURCES "*.cpp")
file(GLOB_RECURSE INTERPRETER_HEADERS "*.hpp")
file(GLOB_RECURSE INTERPRETER_RESOURCES "*.rc")

add_executable(ShardScript.Interpreter
    ${INTERPRETER_SOURCES}
    ${INTERPRETER_HEADERS}
    ${INTERPRETER_RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")

target_link_libraries(ShardScript.Interpreter PRIVATE
    ShardScript
    ShardScript.Framework
    dbghelp
    version
    shlwapi
)

if(MINGW)
    target_link_options(ShardScript.Interpreter PRIVATE -municode)
endif()
