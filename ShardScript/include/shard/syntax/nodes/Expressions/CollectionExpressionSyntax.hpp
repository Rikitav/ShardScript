#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API CollectionExpressionSyntax : public ExpressionSyntax
	{
	public:
		SyntaxToken OpenSquareToken;
		SyntaxToken CloseSquareToken;
		
		std::vector<std::unique_ptr<ExpressionSyntax>> ValuesExpressions;
		ArrayTypeSymbol* Symbol = nullptr;

		inline CollectionExpressionSyntax(SyntaxNode *const parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline CollectionExpressionSyntax(const CollectionExpressionSyntax&) = delete;

		inline virtual ~CollectionExpressionSyntax() = default;
	};
}
