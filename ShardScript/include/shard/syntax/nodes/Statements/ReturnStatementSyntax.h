#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard::syntax::nodes
{
	class SHARD_API ReturnStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ReturnStatementSyntax(const SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ReturnStatement, parent) { }
		inline ReturnStatementSyntax(const ReturnStatementSyntax& other) = delete;

		inline virtual ~ReturnStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
