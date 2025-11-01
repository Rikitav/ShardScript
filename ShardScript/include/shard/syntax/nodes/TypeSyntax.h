#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class TypeSyntax : public SyntaxNode
	{
	public:
		inline TypeSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: SyntaxNode(kind, parent) { }
	};
}