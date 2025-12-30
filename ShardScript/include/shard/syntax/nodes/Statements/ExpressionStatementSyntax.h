#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

namespace shard
{
	class SHARD_API ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ExpressionStatementSyntax(const SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }
		inline ExpressionStatementSyntax(ExpressionSyntax* expression, const SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent), Expression(expression) { }
		inline ExpressionStatementSyntax(const ExpressionStatementSyntax& other) = delete;

		inline virtual ~ExpressionStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
