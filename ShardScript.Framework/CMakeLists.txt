project(ShardScript.Framework)

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "system/*.shard.cpp")

foreach(CPP_FILE IN LISTS CPP_FILES)
    get_filename_component(TARGET_NAME "${CPP_FILE}" NAME_WLE)
    add_library("${TARGET_NAME}" SHARED "${CPP_FILE}")

    set_target_properties("${TARGET_NAME}" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set_target_properties("${TARGET_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/framework")

    target_include_directories("${TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories("${TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")

    target_link_libraries("${TARGET_NAME}" PRIVATE ShardScript)
endforeach()
