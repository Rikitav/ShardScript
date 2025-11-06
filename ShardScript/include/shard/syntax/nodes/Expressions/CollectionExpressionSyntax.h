#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>
#include <vector>

namespace shard::syntax::nodes
{
	class CollectionExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ExpressionSyntax*> ValuesExpressions;

		inline CollectionExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline CollectionExpressionSyntax(const CollectionExpressionSyntax& other)
			: ExpressionSyntax(other), OpenSquareToken(other.OpenSquareToken), CloseSquareToken(other.CloseSquareToken), ValuesExpressions(other.ValuesExpressions) { }

		inline virtual ~CollectionExpressionSyntax()
		{
			for (ExpressionSyntax* expression : ValuesExpressions)
				delete expression;
		}
	};
}
