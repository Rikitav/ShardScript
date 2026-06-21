#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <string>
#include <memory>

namespace shard
{
    class SHARD_API MemberSymbol : public SyntaxSymbol
    {
    public:
        SymbolLinking Linking = SymbolLinking::Instance;

        inline MemberSymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline MemberSymbol(const MemberSymbol& other) = delete;

        inline virtual ~MemberSymbol() = default;

        void OnSymbolDeclared(SyntaxSymbol* symbol) override;
        bool IsType() const override;
        bool IsMember() const override;
    };
}