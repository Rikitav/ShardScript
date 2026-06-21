#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class SHARD_API CastExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		std::unique_ptr<TypeSyntax> TargetType = nullptr;

		inline CastExpressionSyntax(const SyntaxToken operatorToken, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::CastExpression, parent), OperatorToken(operatorToken) {}

		inline CastExpressionSyntax(const CastExpressionSyntax&) = delete;

		inline virtual ~CastExpressionSyntax() = default;
	};
}
