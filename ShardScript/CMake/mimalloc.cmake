# mimalloc source list for direct compilation into ShardScript core.
# Uses the single-TU static build (src/static.c).
# MI_MALLOC_OVERRIDE is intentionally NOT defined: the standard malloc/free entry points stay untouched;
# only explicit mi_* calls use mimalloc.

set(MIMALLOC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc")

set(MIMALLOC_SOURCES
    ${MIMALLOC_ROOT}/src/static.c
)

set(MIMALLOC_DEFINES
    MI_STATIC_LIB
    SHARD_USE_MIMALLOC
)
