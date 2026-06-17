project(ShardScript.Interpreter)

file(GLOB_RECURSE INTERPRETER_SOURCES "*.cpp")
file(GLOB_RECURSE INTERPRETER_HEADERS "*.hpp")

add_executable(ShardScript.Interpreter
    ${INTERPRETER_SOURCES}
    ${INTERPRETER_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")

target_link_libraries(ShardScript.Interpreter PRIVATE
    ShardScript
    #ShardScript.Framework
)

if(MINGW)
    target_link_options(ShardScript.Interpreter PRIVATE -municode)
endif()
