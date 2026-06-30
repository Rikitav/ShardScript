#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

namespace shard
{
    struct SHARD_API TypeInfo
    {
    public:
        shard::TypeSymbol* Type;

        inline TypeInfo(shard::TypeSymbol* type)
            : Type(type) { }
    };
}
