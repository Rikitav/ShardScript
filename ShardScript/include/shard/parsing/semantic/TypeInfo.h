#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/symbols/TypeSymbol.h>

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
