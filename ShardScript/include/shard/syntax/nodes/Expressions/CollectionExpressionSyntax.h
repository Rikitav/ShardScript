#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <vector>

namespace shard
{
	class SHARD_API CollectionExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		std::vector<ExpressionSyntax*> ValuesExpressions;
		shard::ArrayTypeSymbol* Symbol = nullptr;

		inline CollectionExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline CollectionExpressionSyntax(const CollectionExpressionSyntax&) = delete;

		inline virtual ~CollectionExpressionSyntax()
		{
			for (ExpressionSyntax* expression : ValuesExpressions)
				delete expression;
		}
	};
}
