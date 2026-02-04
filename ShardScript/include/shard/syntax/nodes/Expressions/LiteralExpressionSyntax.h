#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxNode.h>

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
