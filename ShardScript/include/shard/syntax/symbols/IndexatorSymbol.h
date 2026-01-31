#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <vector>

namespace shard
{
    class PropertySymbol;

    class SHARD_API IndexatorSymbol : public PropertySymbol
    {
    public:
        std::vector<ParameterSymbol*> Parameters;

        inline IndexatorSymbol(const std::wstring& name)
            : PropertySymbol(name, SyntaxKind::IndexatorDeclaration) { }

        inline IndexatorSymbol(const IndexatorSymbol& other) = delete;

        inline ~IndexatorSymbol() override
        {
            for (ParameterSymbol* param : Parameters)
                delete param;
        }
    };
}

