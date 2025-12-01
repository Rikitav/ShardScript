#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing::semantic
{
    struct SHARD_API TypeInfo
    {
    public:
        shard::syntax::symbols::TypeSymbol* Type;

        inline TypeInfo(shard::syntax::symbols::TypeSymbol* type)
            : Type(type) { }
    };
}
