#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API BreakStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline BreakStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::BreakStatement, parent) {}
		inline BreakStatementSyntax(const BreakStatementSyntax& other) = delete;
		inline virtual ~BreakStatementSyntax() {}
	};
}
