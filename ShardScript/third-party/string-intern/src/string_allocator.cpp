#include "string_allocator.h"

#include <cstdlib>

namespace
{
    rs::stringintern::alloc_func g_malloc = std::malloc;
    rs::stringintern::realloc_func g_realloc = std::realloc;
    rs::stringintern::calloc_func g_calloc = std::calloc;
    rs::stringintern::free_func g_free = std::free;
}

void rs::stringintern::replace_allocator(alloc_func mallocFunc, realloc_func reallocFunc, calloc_func callocFunc, free_func freeFunc)
{
    g_malloc = mallocFunc != nullptr ? mallocFunc : std::malloc;
    g_realloc = reallocFunc != nullptr ? reallocFunc : std::realloc;
    g_calloc = callocFunc != nullptr ? callocFunc : std::calloc;
    g_free = freeFunc != nullptr ? freeFunc : std::free;
}

void* rs::stringintern::Allocate(std::size_t bytes)
{
    return g_malloc(bytes);
}

void* rs::stringintern::Reallocate(void* block, std::size_t bytes)
{
    return g_realloc(block, bytes);
}

void* rs::stringintern::AllocateZeroed(std::size_t count, std::size_t bytes)
{
    return g_calloc(count, bytes);
}

void rs::stringintern::Deallocate(void* block) noexcept
{
    g_free(block);
}
