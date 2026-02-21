#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API ContinueStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline ContinueStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ContinueStatement, parent) {}
		inline ContinueStatementSyntax(const ContinueStatementSyntax& other) = delete;

		inline virtual ~ContinueStatementSyntax() {}
	};
}
