#pragma once
#include <cstdint>
#include <cstring>

#ifndef SHARD_NO_MIMALLOC
	#include <mimalloc.h>
#endif

namespace gmt
{
	inline void* mem_alloc(size_t bytes)
	{
#ifdef SHARD_NO_MIMALLOC
		return std::malloc(bytes);
#else
		return mi_malloc(bytes);
#endif
	}

	inline void* mem_realloc(void* block, size_t bytes)
	{
#ifdef SHARD_NO_MIMALLOC
		return std::realloc(block, bytes);
#else
		return mi_realloc(block, bytes);
#endif
	}

	inline void mem_free(void* ptr)
	{
#ifdef SHARD_NO_MIMALLOC
		return std::free(ptr);
#else
		return mi_free(ptr);
#endif
	}
}
