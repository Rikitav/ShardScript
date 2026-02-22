#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API ReturnStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ReturnStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ReturnStatement, parent) { }
		inline ReturnStatementSyntax(const ReturnStatementSyntax& other) = delete;

		inline virtual ~ReturnStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
