# string-intern (Ripcord Software libstringintern, MIT) sources for direct
# compilation into ShardScript core.

set(STRING_INTERN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/string-intern")

set(STRING_INTERN_SOURCES
    ${STRING_INTERN_ROOT}/string_hash.cpp
    ${STRING_INTERN_ROOT}/string_intern.cpp
    ${STRING_INTERN_ROOT}/string_page.cpp
    ${STRING_INTERN_ROOT}/string_page_archive.cpp
    ${STRING_INTERN_ROOT}/string_page_catalog.cpp
    ${STRING_INTERN_ROOT}/string_page_nursery.cpp
    ${STRING_INTERN_ROOT}/string_page_ptr.cpp
    ${STRING_INTERN_ROOT}/string_page_sizes.cpp
    ${STRING_INTERN_ROOT}/string_pages.cpp
    ${STRING_INTERN_ROOT}/string_reference.cpp
    ${STRING_INTERN_ROOT}/xxhash.c
)
