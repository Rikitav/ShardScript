#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		const bool IsRightDetermined;

		inline UnaryExpressionSyntax(const SyntaxToken operatorToken, const bool isRightDetermined, SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::UnaryExpression, parent), OperatorToken(operatorToken), IsRightDetermined(isRightDetermined) { }

		inline UnaryExpressionSyntax(const UnaryExpressionSyntax&) = delete;

		inline virtual ~UnaryExpressionSyntax() = default;
	};
}
