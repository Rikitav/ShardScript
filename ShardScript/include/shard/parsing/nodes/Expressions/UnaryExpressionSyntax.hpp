#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <memory>

namespace shard
{
	class OperatorSymbol;

	class SHARD_API UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		const bool IsRightDetermined;
		OperatorSymbol* ToOperator = nullptr;

		inline UnaryExpressionSyntax(const SyntaxToken operatorToken, const bool isRightDetermined, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::UnaryExpression, parent), OperatorToken(operatorToken), IsRightDetermined(isRightDetermined) { }

		inline UnaryExpressionSyntax(const UnaryExpressionSyntax&) = delete;

		inline virtual ~UnaryExpressionSyntax() = default;
	};
}
