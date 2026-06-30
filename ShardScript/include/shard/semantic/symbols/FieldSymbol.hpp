#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <string>
#include <cstdint>

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