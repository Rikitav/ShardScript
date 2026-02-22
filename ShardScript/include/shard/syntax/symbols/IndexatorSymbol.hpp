#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>

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

