#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

namespace shard
{
	class SHARD_API UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Expression = nullptr;
		const bool IsRightDetermined;

		inline UnaryExpressionSyntax(const SyntaxToken operatorToken, const bool isRightDetermined, const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::UnaryExpression, parent), OperatorToken(operatorToken), IsRightDetermined(isRightDetermined) { }

		inline UnaryExpressionSyntax(const UnaryExpressionSyntax&) = delete;

		inline virtual ~UnaryExpressionSyntax()
		{
			delete Expression;
		}
	};
}
