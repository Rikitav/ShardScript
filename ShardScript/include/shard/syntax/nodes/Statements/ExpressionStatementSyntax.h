#pragma once
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

namespace shard::syntax::nodes
{
	class ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		ExpressionSyntax* Expression = nullptr;

		inline ExpressionStatementSyntax(const SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }
		inline ExpressionStatementSyntax(ExpressionSyntax* expression, const SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent), Expression(expression) { }
		inline ExpressionStatementSyntax(const ExpressionStatementSyntax& other) : StatementSyntax(other), Expression(other.Expression) { }

		inline virtual ~ExpressionStatementSyntax()
		{
			if (Expression != nullptr)
			{
				Expression->~ExpressionSyntax();
				delete Expression;
			}
		}
	};
}
