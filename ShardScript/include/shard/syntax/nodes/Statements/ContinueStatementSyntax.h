#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard::syntax::nodes
{
	class SHARD_API ContinueStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline ContinueStatementSyntax(const SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ContinueStatement, parent) {}
		inline ContinueStatementSyntax(const ContinueStatementSyntax& other) = delete;

		inline virtual ~ContinueStatementSyntax() {}
	};
}
