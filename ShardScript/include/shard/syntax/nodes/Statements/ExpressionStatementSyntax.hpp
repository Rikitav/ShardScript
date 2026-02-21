#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

namespace shard
{
	class SHARD_API ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ExpressionStatementSyntax(SyntaxNode *const parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }
		inline ExpressionStatementSyntax(ExpressionSyntax* expression, SyntaxNode *const parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent), Expression(expression) { }
		inline ExpressionStatementSyntax(const ExpressionStatementSyntax& other) = delete;

		inline virtual ~ExpressionStatementSyntax()
		{
			if (Expression != nullptr)
				delete Expression;
		}
	};
}
