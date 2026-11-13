# cppdap 1.65.0 (Apache-2.0), using nlohmann/json 3.11.3 (MIT).

set(CPPDAP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third-party/cppdap")

set(CPPDAP_SOURCES
    ${CPPDAP_ROOT}/src/content_stream.cpp
    ${CPPDAP_ROOT}/src/io.cpp
    ${CPPDAP_ROOT}/src/nlohmann_json_serializer.cpp
    ${CPPDAP_ROOT}/src/network.cpp
    ${CPPDAP_ROOT}/src/null_json_serializer.cpp
    ${CPPDAP_ROOT}/src/protocol_events.cpp
    ${CPPDAP_ROOT}/src/protocol_requests.cpp
    ${CPPDAP_ROOT}/src/protocol_response.cpp
    ${CPPDAP_ROOT}/src/protocol_types.cpp
    ${CPPDAP_ROOT}/src/session.cpp
    ${CPPDAP_ROOT}/src/socket.cpp
    ${CPPDAP_ROOT}/src/typeinfo.cpp
    ${CPPDAP_ROOT}/src/typeof.cpp
)

set(CPPDAP_INCLUDE_DIRS
    "${CPPDAP_ROOT}/include"
    "${CMAKE_CURRENT_LIST_DIR}/../third-party/nlohmann-json/include"
)

set(CPPDAP_DEFINES CPPDAP_JSON_NLOHMANN=1)

if(WIN32)
    set(CPPDAP_LIBRARIES ws2_32)
else()
    find_package(Threads REQUIRED)
    set(CPPDAP_LIBRARIES Threads::Threads)
endif()

set_source_files_properties(${CPPDAP_SOURCES} PROPERTIES COMPILE_FLAGS "-w")
