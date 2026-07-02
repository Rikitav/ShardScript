#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class OperatorSymbol;

	class SHARD_API CastExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		std::unique_ptr<TypeSyntax> TargetType = nullptr;

		OperatorSymbol* ToOperator = nullptr;
		bool IsPrimitiveCast = false;

		inline CastExpressionSyntax(const SyntaxToken operatorToken, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::CastExpression, parent), OperatorToken(operatorToken) {}

		inline CastExpressionSyntax(const CastExpressionSyntax&) = delete;

		inline virtual ~CastExpressionSyntax() = default;
	};
}
