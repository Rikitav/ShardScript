#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MemberSymbol.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <string>
#include <memory>

namespace shard
{
    class SHARD_API PropertySymbol : public MemberSymbol
    {
    public:
        FieldSymbol* BackingField = nullptr;
        AccessorSymbol* Getter = nullptr;
        AccessorSymbol* Setter = nullptr;
        ExpressionSyntax* DefaultValueExpression = nullptr;
        
        TypeSymbol* ReturnType = nullptr;
        bool IsStatic = false;

        inline PropertySymbol(const std::wstring& name)
            : MemberSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline PropertySymbol(const std::wstring& name, const SyntaxKind kind)
            : MemberSymbol(name, kind) { }

        inline PropertySymbol(const PropertySymbol& other) = delete;

        inline virtual ~PropertySymbol() = default;
    };
}
