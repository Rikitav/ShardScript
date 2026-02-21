#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>

namespace shard
{
	class SHARD_API ThrowStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ThrowStatementSyntax(SyntaxNode *const parent) : KeywordStatementSyntax(SyntaxKind::ThrowStatement, parent) {}
		inline ThrowStatementSyntax(const ThrowStatementSyntax& other) = delete;

		inline virtual ~ThrowStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
