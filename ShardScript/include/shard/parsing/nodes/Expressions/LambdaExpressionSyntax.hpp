#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <memory>

namespace shard
{
    class SHARD_API LambdaExpressionSyntax : public ExpressionSyntax
    {
    public:
        SyntaxToken LambdaToken;
        SyntaxToken LambdaOperatorToken;
        SyntaxToken ReturnTypeArrowToken;
        SyntaxToken AsyncModifierToken;   // empty if not async

        std::unique_ptr<ParametersListSyntax> ParametersList = nullptr;
        std::unique_ptr<TypeSyntax> ReturnType = nullptr;
        std::unique_ptr<StatementsBlockSyntax> Body = nullptr;
        DelegateTypeSymbol* Symbol = nullptr;

        inline LambdaExpressionSyntax(SyntaxNode* parent)
            : ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

        inline LambdaExpressionSyntax(const LambdaExpressionSyntax&) = delete;

        inline virtual ~LambdaExpressionSyntax() override = default;
    };
}
