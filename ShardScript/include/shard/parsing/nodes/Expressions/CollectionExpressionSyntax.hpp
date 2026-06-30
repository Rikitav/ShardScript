#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

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

		inline CollectionExpressionSyntax(SyntaxNode* parent)
			: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

		inline CollectionExpressionSyntax(const CollectionExpressionSyntax&) = delete;

		inline virtual ~CollectionExpressionSyntax() = default;
	};
}
