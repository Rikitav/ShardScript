project(ShardScript.Framework)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(SCRIPT_FILES
        "./system/collections/List.ss"
)

set(GENERATED_HEADERS "")
foreach(SCRIPT ${SCRIPT_FILES})
    get_filename_component(SCRIPT_NAME ${SCRIPT} NAME_WE)
    set(OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/${SCRIPT_NAME}_compiled.h")

    # Имя переменной, например: script_List_ss
    set(VAR_NAME "script_${SCRIPT_NAME}_ss")

    add_custom_command(
            OUTPUT ${OUTPUT_HEADER}
            COMMAND ${CMAKE_COMMAND}
            -DIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}"
            -DOUT_HEADER="${OUTPUT_HEADER}"
            -DVAR_NAME="${VAR_NAME}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/../CMake/HexGenerator.cmake"
            DEPENDS ${SCRIPT}
            COMMENT "Generating cross-platform hex header for ${SCRIPT_NAME}.ss"
    )
    list(APPEND GENERATED_HEADERS ${OUTPUT_HEADER})
endforeach()

file(GLOB_RECURSE FRAMEWORK_SOURCES "*.cpp")
file(GLOB_RECURSE FRAMEWORK_HEADERS "*.hpp")

add_library(ShardScript.Framework SHARED 
    ${FRAMEWORK_SOURCES} 
    ${FRAMEWORK_HEADERS}
    ${GENERATED_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-build-debug/ShardScript.Framework/generated")

target_link_libraries(ShardScript.Framework PUBLIC
    ShardScript
)

set_target_properties(ShardScript.Framework PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)