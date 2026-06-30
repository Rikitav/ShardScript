#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <vector>
#include <memory>

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

        inline ~IndexatorSymbol() override = default;

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;
    };
}

