/**
 * Allocator replacement hook, modelled on libuv's uv_replace_allocator().
 *
 * The default allocator is the C runtime. Call replace_allocator() before the
 * first string-intern allocation: pages allocated with one allocator must be
 * released by the same allocator.
**/

#ifndef RS_LIBSTRINGINTERN_STRING_ALLOCATOR_H
#define RS_LIBSTRINGINTERN_STRING_ALLOCATOR_H

#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <vector>

namespace rs {
namespace stringintern {

using alloc_func = void* (*)(std::size_t);
using realloc_func = void* (*)(void*, std::size_t);
using calloc_func = void* (*)(std::size_t, std::size_t);
using free_func = void (*)(void*);

void replace_allocator(
    alloc_func mallocFunc,
    realloc_func reallocFunc,
    calloc_func callocFunc,
    free_func freeFunc
);

void* Allocate(std::size_t bytes);
void* Reallocate(void* block, std::size_t bytes);
void* AllocateZeroed(std::size_t count, std::size_t bytes);
void Deallocate(void* block) noexcept;

template<typename T>
struct StringAllocator
{
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    StringAllocator() noexcept = default;
    template<typename U> StringAllocator(const StringAllocator<U>&) noexcept {}

    T* allocate(std::size_t count)
    {
        if (count > max_size())
            throw std::bad_array_new_length();

        auto block = Allocate(count * sizeof(T));
        if (block == nullptr)
            throw std::bad_alloc();

        return static_cast<T*>(block);
    }

    void deallocate(T* block, std::size_t) noexcept
    {
        Deallocate(block);
    }

    std::size_t max_size() const noexcept
    {
        return (std::numeric_limits<std::size_t>::max)() / sizeof(T);
    }
};

template<typename T, typename U>
bool operator==(const StringAllocator<T>&, const StringAllocator<U>&) noexcept
{
    return true;
}

template<typename T, typename U>
bool operator!=(const StringAllocator<T>&, const StringAllocator<U>&) noexcept
{
    return false;
}

template<typename T>
using StringVector = std::vector<T, StringAllocator<T>>;

}}

#endif /* RS_LIBSTRINGINTERN_STRING_ALLOCATOR_H */
