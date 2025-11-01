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
		
		inline BinaryExpressionSyntax(const BinaryExpressionSyntax& other)
			: ExpressionSyntax(other), Left(other.Left), OperatorToken(other.OperatorToken), Right(other.Right) { }

		inline virtual ~BinaryExpressionSyntax()
		{
			Left->~ExpressionSyntax();
			Right->~ExpressionSyntax();

			delete Left;
			delete Right;
		}
	};
}
