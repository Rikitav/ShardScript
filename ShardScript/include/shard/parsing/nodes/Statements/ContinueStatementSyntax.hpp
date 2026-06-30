#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API ContinueStatementSyntax : public KeywordStatementSyntax
	{
	public:
		inline ContinueStatementSyntax(SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ContinueStatement, parent) {}
		inline ContinueStatementSyntax(const ContinueStatementSyntax& other) = delete;

		inline virtual ~ContinueStatementSyntax() {}
	};
}
