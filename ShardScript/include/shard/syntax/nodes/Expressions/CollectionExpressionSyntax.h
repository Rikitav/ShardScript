#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
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
		shard::syntax::symbols::ArrayTypeSymbol* Symbol = nullptr;

		inline CollectionExpressionSyntax(const SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline virtual ~CollectionExpressionSyntax()
		{
			for (ExpressionSyntax* expression : ValuesExpressions)
				delete expression;
		}
	};
}
