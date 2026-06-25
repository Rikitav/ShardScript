project(ShardScript.Framework)

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
    "system/*.shard.cpp"
    "third_party/*.shard.cpp"
)

foreach(CPP_FILE IN LISTS CPP_FILES)
  get_filename_component(TARGET_NAME "${CPP_FILE}" NAME_WLE)
  add_library("${TARGET_NAME}" SHARED "${CPP_FILE}")

  set_target_properties("${TARGET_NAME}" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

  # The top-level CMake sets CMAKE_(RUNTIME|LIBRARY)_OUTPUT_DIRECTORY_<CONFIG>,
  # which takes precedence over the plain output-directory properties. Make sure
  # framework libraries land in bin/framework for every configuration, on both
  # Windows (DLLs are RUNTIME) and Linux/Unix (shared objects are LIBRARY).
  set(FRAMEWORK_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/framework")
  set_target_properties("${TARGET_NAME}" PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${FRAMEWORK_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FRAMEWORK_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FRAMEWORK_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FRAMEWORK_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FRAMEWORK_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY "${FRAMEWORK_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${FRAMEWORK_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FRAMEWORK_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FRAMEWORK_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${FRAMEWORK_OUTPUT_DIR}"
  )

  target_include_directories("${TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories("${TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")

  target_link_libraries("${TARGET_NAME}" PRIVATE ShardScript)
endforeach()
