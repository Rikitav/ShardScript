#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

namespace shard::syntax::nodes
{
	class ExpressionSyntax : public SyntaxNode
	{
	public:
		inline ExpressionSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }

		inline ExpressionSyntax(const ExpressionSyntax& other)
			: SyntaxNode(other) { }

		inline virtual ~ExpressionSyntax()
		{

		}
	};
}
