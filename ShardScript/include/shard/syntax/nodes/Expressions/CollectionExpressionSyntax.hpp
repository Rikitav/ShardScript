#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
