#include <gmt/Allocator.hpp>

#include <cstdlib>

#ifndef SHARD_NO_MIMALLOC
	#include <mimalloc.h>
#endif

void* gmt::mem_alloc(std::size_t bytes)
{
#ifdef SHARD_NO_MIMALLOC
	return std::malloc(bytes);
#else
	return mi_malloc(bytes);
#endif
}

void* gmt::mem_alloc_aligned(std::size_t bytes, std::size_t alignment)
{
#ifdef SHARD_NO_MIMALLOC
	const std::size_t rounded = (bytes + alignment - 1) / alignment * alignment;
	return std::aligned_alloc(alignment, rounded);
#else
	return mi_malloc_aligned(bytes, alignment);
#endif
}

void* gmt::mem_realloc(void* block, std::size_t bytes)
{
#ifdef SHARD_NO_MIMALLOC
	return std::realloc(block, bytes);
#else
	return mi_realloc(block, bytes);
#endif
}

void gmt::mem_free(void* ptr)
{
	if (ptr == nullptr)
		return;

#ifdef SHARD_NO_MIMALLOC
	std::free(ptr);
#else
	mi_free(ptr);
#endif
}
