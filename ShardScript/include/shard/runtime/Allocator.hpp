#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <cstdlib>
#include <cstddef>

#if defined(SHARD_USE_MIMALLOC)
#include <mimalloc.h>
#endif

namespace shard
{
    // Central allocation entry points for engine-owned raw buffers (instance payloads,
    // string buffers, small-int cache slab, etc.).
    //
    // Routed through mimalloc when compiled into the engine (SHARD_USE_MIMALLOC,
    // defined only for the ShardScript target); plain malloc/free otherwise.
    //
    // WARNING: a buffer returned here must be released with FreeBytes() from the same
    // module that allocated it. Never mix with std::free outside the engine.

    inline void* AllocateBytes(std::size_t size)
    {
#if defined(SHARD_USE_MIMALLOC)
        return mi_malloc(size);
#else
        return std::malloc(size);
#endif
    }

    inline void* AllocateZeroedBytes(std::size_t size)
    {
#if defined(SHARD_USE_MIMALLOC)
        return mi_zalloc(size);
#else
        return std::calloc(1, size);
#endif
    }

    inline void FreeBytes(void* memory)
    {
#if defined(SHARD_USE_MIMALLOC)
        mi_free(memory);
#else
        std::free(memory);
#endif
    }
}
