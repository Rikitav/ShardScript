#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard::syntax::nodes
{
	class BreakStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline BreakStatementSyntax(const SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::BreakStatement, parent) {}
		inline BreakStatementSyntax(const BreakStatementSyntax& other) : KeywordStatementSyntax(other) {}

		inline virtual ~BreakStatementSyntax() {}
	};
}
