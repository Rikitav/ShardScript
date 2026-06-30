#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <string>
#include <memory>

namespace shard
{
    class SHARD_API PropertySymbol : public MemberSymbol
    {
    public:
        FieldSymbol* BackingField = nullptr;
        TypeSymbol* ReturnType = nullptr;
        AccessorSymbol* Getter = nullptr;
        AccessorSymbol* Setter = nullptr;
        
        ExpressionSyntax* DefaultValueExpression = nullptr;

        inline PropertySymbol(const std::wstring& name)
            : MemberSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline PropertySymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind) { }

        inline PropertySymbol(const PropertySymbol& other) = delete;

        inline virtual ~PropertySymbol() = default;
    };
}
