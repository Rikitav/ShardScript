#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

namespace shard::syntax::nodes
{
	class UnaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Expression = nullptr;
		const bool IsRightDetermined;

		inline UnaryExpressionSyntax(const SyntaxToken operatorToken, const bool isRightDetermined, const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::UnaryExpression, parent), OperatorToken(operatorToken), IsRightDetermined(isRightDetermined) { }

		UnaryExpressionSyntax(const UnaryExpressionSyntax&) = delete;

		inline virtual ~UnaryExpressionSyntax()
		{
			Expression->~ExpressionSyntax();
			delete Expression;
		}
	};
}
