project(ShardScript.Framework)

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
    "system/*.shard.cpp"
    "system/**/*.shard.cpp"
    "third_party/**/*.shard.cpp"
)

foreach(CPP_FILE IN LISTS CPP_FILES)
  get_filename_component(SHARD_TARGET_NAME "${CPP_FILE}" NAME_WLE)
  add_library("${SHARD_TARGET_NAME}" SHARED "${CPP_FILE}")
  
if(ENABLE_ASAN)
    if(MSVC)
        target_compile_options(${SHARD_TARGET_NAME} PRIVATE /fsanitize=address)
        target_link_options(${SHARD_TARGET_NAME} PRIVATE /fsanitize=address)
    else()
        target_compile_options(${SHARD_TARGET_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${SHARD_TARGET_NAME} PRIVATE -fsanitize=address)
    endif()
endif()

  set_target_properties("${SHARD_TARGET_NAME}" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

  string(FIND "${CPP_FILE}" "/system/" IS_SYSTEM)
  if(NOT IS_SYSTEM EQUAL -1)
    set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/system")
  else()
    set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/third-party")
  endif()
  
  set_target_properties("${SHARD_TARGET_NAME}" PROPERTIES
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
  )

  string (FIND "${SHARD_TARGET_NAME}" "http" IS_HTTP_TARGET)
  if (NOT IS_HTTP_TARGET EQUAL -1)
    if(WIN32)
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/system/lib/openssl.lib" ssl crypto ws2_32 gdi32 crypt32)
    else()
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE ssl crypto)
    endif()
  endif()

  # RayLib target
  string(FIND "${SHARD_TARGET_NAME}" "raylib" IS_RAYLIB_TARGET)
  if(NOT IS_RAYLIB_TARGET EQUAL -1)
    target_include_directories("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include")
    
    if(WIN32)
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/RayLib/lib/raylib.lib" winmm gdi32 shell32 user32)
    else()
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/RayLib/lib/libraylib.a" GL m pthread dl rt X11)
    endif()
  endif()
  
  # SQLite target
  string(FIND "${SHARD_TARGET_NAME}" "sqlite" IS_SQLITE_TARGET)
  if(NOT IS_SQLITE_TARGET EQUAL -1)
    target_include_directories("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SQLite3/include")
    
    if(MSVC)
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SQLite3/lib/sqlite3.lib")
    else()
        target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SQLite3/lib/libsqlite3.a")
    endif()
  endif()

  target_include_directories("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories("${SHARD_TARGET_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ShardScript/include")
  target_link_libraries("${SHARD_TARGET_NAME}" PRIVATE ShardScript)
endforeach()