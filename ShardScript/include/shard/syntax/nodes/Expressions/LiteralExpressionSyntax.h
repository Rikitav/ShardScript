#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class LiteralExpressionSyntax : public ExpressionSyntax
	{
	public:
		const SyntaxToken LiteralToken;

		inline LiteralExpressionSyntax(const SyntaxToken literal, const SyntaxNode* parent) : ExpressionSyntax(SyntaxKind::LiteralExpression, parent), LiteralToken(literal) { }
		LiteralExpressionSyntax(const LiteralExpressionSyntax&) = delete;
		inline virtual ~LiteralExpressionSyntax() { }
	};
}
