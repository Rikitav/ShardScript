#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <memory>

namespace shard
{
    class MethodSymbol;
    class PropertySymbol;
    class TypeSymbol;

    class SHARD_API AwaitExpressionSyntax : public ExpressionSyntax
    {
    public:
        SyntaxToken AwaitKeywordToken;
        std::unique_ptr<ExpressionSyntax> Expression = nullptr;

        // Populated by semantic analysis (awaitable pattern resolution).
        MethodSymbol* GetAwaiterMethod = nullptr;       // null for self-awaiters
        TypeSymbol* AwaiterType = nullptr;
        MethodSymbol* OnCompletedMethod = nullptr;
        PropertySymbol* IsCompletedProperty = nullptr;
        MethodSymbol* IsCompletedMethod = nullptr;
        MethodSymbol* GetResultMethod = nullptr;

        inline AwaitExpressionSyntax(SyntaxToken awaitToken, SyntaxNode* parent)
            : ExpressionSyntax(SyntaxKind::AwaitExpression, parent),
              AwaitKeywordToken(awaitToken) {}

        inline AwaitExpressionSyntax(const AwaitExpressionSyntax&) = delete;

        inline virtual ~AwaitExpressionSyntax() override = default;
    };
}
