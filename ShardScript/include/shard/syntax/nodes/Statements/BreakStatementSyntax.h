#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard
{
	class SHARD_API BreakStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline BreakStatementSyntax(const SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::BreakStatement, parent) {}
		inline BreakStatementSyntax(const BreakStatementSyntax& other) = delete;
		inline virtual ~BreakStatementSyntax() {}
	};
}
