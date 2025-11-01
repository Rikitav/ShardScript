#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing::semantic
{
    struct TypeInfo
    {
    public:
        shard::syntax::symbols::TypeSymbol* Type;

        inline TypeInfo(shard::syntax::symbols::TypeSymbol* type)
            : Type(type) { }
    };
}
