#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API BreakStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline BreakStatementSyntax(SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::BreakStatement, parent) {}
		inline BreakStatementSyntax(const BreakStatementSyntax& other) = delete;
		inline virtual ~BreakStatementSyntax() {}
	};
}
