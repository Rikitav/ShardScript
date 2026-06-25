#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MemberSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <string>
#include <memory>

namespace shard
{
    class SHARD_API FieldSymbol : public MemberSymbol
    {
    public:
        ExpressionSyntax* DefaultValueExpression = nullptr;
        std::size_t MemoryBytesOffset = 0;
        TypeSymbol* ReturnType = nullptr;

        bool IsEnumValue = false;
        std::int64_t EnumValue = 0;

        inline FieldSymbol(const std::wstring& name)
            : MemberSymbol(name, SyntaxKind::FieldDeclaration) { }

        inline FieldSymbol(const FieldSymbol& other) = delete;

        inline virtual ~FieldSymbol() = default;
    };
}