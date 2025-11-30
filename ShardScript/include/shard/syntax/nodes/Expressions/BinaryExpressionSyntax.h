#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class BinaryExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken OperatorToken;
		ExpressionSyntax* Left = nullptr;
		ExpressionSyntax* Right = nullptr;

		inline BinaryExpressionSyntax(const SyntaxToken operatorToken, const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::BinaryExpression, parent), OperatorToken(operatorToken) {}
		
		BinaryExpressionSyntax(const BinaryExpressionSyntax&) = delete;

		inline virtual ~BinaryExpressionSyntax()
		{
			delete Left;
			delete Right;
		}
	};
}
