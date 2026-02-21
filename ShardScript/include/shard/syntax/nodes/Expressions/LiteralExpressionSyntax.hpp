#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API LiteralExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken LiteralToken;

		inline LiteralExpressionSyntax(const SyntaxToken literal, SyntaxNode *const parent) : ExpressionSyntax(SyntaxKind::LiteralExpression, parent), LiteralToken(literal) { }
		inline LiteralExpressionSyntax(const LiteralExpressionSyntax&) = delete;
		inline virtual ~LiteralExpressionSyntax() { }
	};
}
