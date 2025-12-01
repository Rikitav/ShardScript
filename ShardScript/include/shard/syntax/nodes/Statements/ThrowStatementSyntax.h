#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard::syntax::nodes
{
	class SHARD_API ThrowStatementSyntax : public KeywordStatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ThrowStatementSyntax(const SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ThrowStatement, parent) {}
		inline ThrowStatementSyntax(const ThrowStatementSyntax& other) = delete;

		inline virtual ~ThrowStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
