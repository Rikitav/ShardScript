project(ShardScript.Interpreter)

set(INTERPRETER_SOURCES
	ShardScript.Interpreter.cpp
	StandardLibrary.cpp
	utilities/impls/flag.cpp
	utilities/impls/glob.cpp
	utilities/impls/whereami.cpp
)

set(INTERPRETER_HEADERS
	StandardLibrary.hpp
	utilities/flag.h
	utilities/glob.h
	utilities/whereami.h
)

if(WIN32)
	set(INTERPRETER_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/res/resources.rc")
endif()

add_executable(ShardScript.Interpreter
	${INTERPRETER_SOURCES}
	${INTERPRETER_HEADERS}
	${INTERPRETER_RESOURCES}
)

set_target_properties(ShardScript.Interpreter PROPERTIES OUTPUT_NAME "shard")

if(ENABLE_ASAN)
	if(MSVC)
		target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
		target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
	else()
		target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
		target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
	endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(ShardScript.Interpreter PROPERTIES
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

target_link_libraries(ShardScript.Interpreter PRIVATE
	ShardScript
)

if(WIN32)
	add_custom_command(TARGET ShardScript.Interpreter POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:ShardScript>
			$<TARGET_FILE_DIR:ShardScript.Interpreter>
	)
endif()
