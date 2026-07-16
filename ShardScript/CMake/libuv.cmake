# libuv source list for direct compilation into ShardScript core.
# Based on libuv 1.52.1 CMakeLists.txt.

set(LIBUV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv")

set(LIBUV_SOURCES
    ${LIBUV_ROOT}/src/fs-poll.c
    ${LIBUV_ROOT}/src/idna.c
    ${LIBUV_ROOT}/src/inet.c
    ${LIBUV_ROOT}/src/random.c
    ${LIBUV_ROOT}/src/strscpy.c
    ${LIBUV_ROOT}/src/strtok.c
    ${LIBUV_ROOT}/src/thread-common.c
    ${LIBUV_ROOT}/src/threadpool.c
    ${LIBUV_ROOT}/src/timer.c
    ${LIBUV_ROOT}/src/uv-common.c
    ${LIBUV_ROOT}/src/uv-data-getter-setters.c
    ${LIBUV_ROOT}/src/version.c
)

set(LIBUV_DEFINES)
set(LIBUV_LIBRARIES)

if(WIN32)
    list(APPEND LIBUV_DEFINES
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00
        _CRT_DECLARE_NONSTDC_NAMES=0
    )

    list(APPEND LIBUV_LIBRARIES
        psapi
        user32
        advapi32
        iphlpapi
        userenv
        ws2_32
        dbghelp
        ole32
        shell32
    )

    list(APPEND LIBUV_SOURCES
        ${LIBUV_ROOT}/src/win/async.c
        ${LIBUV_ROOT}/src/win/core.c
        ${LIBUV_ROOT}/src/win/detect-wakeup.c
        ${LIBUV_ROOT}/src/win/dl.c
        ${LIBUV_ROOT}/src/win/error.c
        ${LIBUV_ROOT}/src/win/fs.c
        ${LIBUV_ROOT}/src/win/fs-event.c
        ${LIBUV_ROOT}/src/win/getaddrinfo.c
        ${LIBUV_ROOT}/src/win/getnameinfo.c
        ${LIBUV_ROOT}/src/win/handle.c
        ${LIBUV_ROOT}/src/win/loop-watcher.c
        ${LIBUV_ROOT}/src/win/pipe.c
        ${LIBUV_ROOT}/src/win/thread.c
        ${LIBUV_ROOT}/src/win/poll.c
        ${LIBUV_ROOT}/src/win/process.c
        ${LIBUV_ROOT}/src/win/process-stdio.c
        ${LIBUV_ROOT}/src/win/signal.c
        ${LIBUV_ROOT}/src/win/snprintf.c
        ${LIBUV_ROOT}/src/win/stream.c
        ${LIBUV_ROOT}/src/win/tcp.c
        ${LIBUV_ROOT}/src/win/tty.c
        ${LIBUV_ROOT}/src/win/udp.c
        ${LIBUV_ROOT}/src/win/util.c
        ${LIBUV_ROOT}/src/win/winapi.c
        ${LIBUV_ROOT}/src/win/winsock.c
    )
else()
    list(APPEND LIBUV_DEFINES
        _FILE_OFFSET_BITS=64
        _LARGEFILE_SOURCE
    )

    if(NOT CMAKE_SYSTEM_NAME MATCHES "Android|OS390|QNX")
        list(APPEND LIBUV_LIBRARIES pthread)
    endif()

    list(APPEND LIBUV_SOURCES
        ${LIBUV_ROOT}/src/unix/async.c
        ${LIBUV_ROOT}/src/unix/core.c
        ${LIBUV_ROOT}/src/unix/dl.c
        ${LIBUV_ROOT}/src/unix/fs.c
        ${LIBUV_ROOT}/src/unix/getaddrinfo.c
        ${LIBUV_ROOT}/src/unix/getnameinfo.c
        ${LIBUV_ROOT}/src/unix/loop-watcher.c
        ${LIBUV_ROOT}/src/unix/loop.c
        ${LIBUV_ROOT}/src/unix/pipe.c
        ${LIBUV_ROOT}/src/unix/poll.c
        ${LIBUV_ROOT}/src/unix/process.c
        ${LIBUV_ROOT}/src/unix/random-devurandom.c
        ${LIBUV_ROOT}/src/unix/signal.c
        ${LIBUV_ROOT}/src/unix/stream.c
        ${LIBUV_ROOT}/src/unix/tcp.c
        ${LIBUV_ROOT}/src/unix/thread.c
        ${LIBUV_ROOT}/src/unix/tty.c
        ${LIBUV_ROOT}/src/unix/udp.c
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND LIBUV_DEFINES _GNU_SOURCE _POSIX_C_SOURCE=200112)
        list(APPEND LIBUV_LIBRARIES dl rt)
        list(APPEND LIBUV_SOURCES
            ${LIBUV_ROOT}/src/unix/linux.c
            ${LIBUV_ROOT}/src/unix/procfs-exepath.c
            ${LIBUV_ROOT}/src/unix/random-getrandom.c
            ${LIBUV_ROOT}/src/unix/random-sysctl-linux.c
        )
    elseif(APPLE)
        list(APPEND LIBUV_DEFINES _DARWIN_UNLIMITED_SELECT=1 _DARWIN_USE_64_BIT_INODE=1)
        list(APPEND LIBUV_SOURCES
            ${LIBUV_ROOT}/src/unix/darwin-proctitle.c
            ${LIBUV_ROOT}/src/unix/darwin.c
            ${LIBUV_ROOT}/src/unix/fsevents.c
        )
    endif()

    if(APPLE OR CMAKE_SYSTEM_NAME MATCHES "Android|Linux")
        list(APPEND LIBUV_SOURCES ${LIBUV_ROOT}/src/unix/proctitle.c)
    endif()
endif()
