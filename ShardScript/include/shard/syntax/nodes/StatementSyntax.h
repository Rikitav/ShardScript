#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class StatementSyntax : public SyntaxNode
	{
	public:
		SyntaxToken Semicolon;

		StatementSyntax(SyntaxKind kind)
			: SyntaxNode(kind) {}
	};
}
