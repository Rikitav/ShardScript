cmake_minimum_required(VERSION 3.12)

project(lsp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /bigobj)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  option(LSP_USE_SANITIZERS "Use address and undefined behavior sanitizers for Debug builds" OFF)

  add_compile_options(
    -Wall
    -Wextra
    -pedantic
    -Wsign-conversion
    -Wfloat-conversion
  )

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
      -Wimplicit-int-conversion
      -Wimplicit-float-conversion
    )
  endif()

  if(LSP_USE_SANITIZERS)
    add_compile_options(
      $<$<CONFIG:DEBUG>:-fsanitize=address,undefined>
      $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>
    )

    add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined>)
  endif()

  if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wa,-mbig-obj)
  endif()
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(lspgen lspgen/lspgen.cpp lsp/json/json.cpp)
target_include_directories(lspgen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(LSP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LSP_SRC_DIR ${LSP_DIR}/lsp)

set(LSP_HEADERS
  # lsp
  concepts.h
  connection.h
  enumeration.h
  error.h
  exception.h
  fileuri.h
  messagebase.h
  messagehandler.h
  nullable.h
  process.h
  requestresult.h
  serialization.h
  strmap.h
  threadpool.h
  uri.h
  # io
  io/socket.h
  io/standardio.h
  io/stream.h
  # json
  json/json.h
  # jsonrpc
  jsonrpc/jsonrpc.h
)

set(LSP_SOURCES
  # lsp
  connection.cpp
  fileuri.cpp
  messagehandler.cpp
  process.cpp
  threadpool.cpp
  uri.cpp
  # io
  io/socket.cpp
  io/standardio.cpp
  # json
  json/json.cpp
  # jsonrpc
  jsonrpc/jsonrpc.cpp
)

list(TRANSFORM LSP_HEADERS PREPEND ${LSP_SRC_DIR}/)
list(TRANSFORM LSP_SOURCES PREPEND ${LSP_SRC_DIR}/)

set(LSP_GENERATED_FILES_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${LSP_GENERATED_FILES_DIR}/lsp)

set(LSP_GENERATED_HEADERS
  ${LSP_GENERATED_FILES_DIR}/lsp/types.h
  ${LSP_GENERATED_FILES_DIR}/lsp/messages.h
)

set(LSP_GENERATED_SOURCES
	${LSP_GENERATED_FILES_DIR}/lsp/types.cpp
)

set(LSP_METAMODEL_JSON ${CMAKE_CURRENT_SOURCE_DIR}/lspgen/metaModel.json)

add_custom_command(
	COMMAND $<TARGET_FILE:lspgen> ${LSP_METAMODEL_JSON}
	WORKING_DIRECTORY ${LSP_GENERATED_FILES_DIR}/lsp
	OUTPUT ${LSP_GENERATED_HEADERS} ${LSP_GENERATED_SOURCES}
	DEPENDS lspgen ${LSP_METAMODEL_JSON}
	COMMENT "Generating lsp types from meta model..."
	VERBATIM
)

add_executable(lsp
  ${LSP_HEADERS}
  ${LSP_SOURCES}
  ${LSP_GENERATED_HEADERS}
  ${LSP_GENERATED_SOURCES}
  ${LSP_DIR}/ShardScript.LspServer.cpp
)

target_include_directories(lsp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${LSP_GENERATED_FILES_DIR}>
)

set_target_properties(lsp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(lsp PRIVATE ShardScript)

if(ENABLE_ASAN)
  if(MSVC)
    target_compile_options(lsp PRIVATE /fsanitize=address)
    target_link_options(lsp PRIVATE /fsanitize=address)
  else()
    target_compile_options(lsp PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(lsp PRIVATE -fsanitize=address)
  endif()
endif()

if(WIN32)
    target_link_libraries(lsp PRIVATE ws2_32)
endif()

if(MSVC AND ENABLE_ASAN)
    get_filename_component(_asan_dir ${CMAKE_CXX_COMPILER} DIRECTORY)
    add_custom_command(TARGET lsp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_asan_dir}/clang_rt.asan_dynamic-x86_64.dll"
            $<TARGET_FILE_DIR:lsp>
        COMMENT "Copying ASan runtime next to lsp.exe"
    )
endif()