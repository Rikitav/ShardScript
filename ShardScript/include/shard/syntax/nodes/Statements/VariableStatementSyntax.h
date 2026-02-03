#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/TypeSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard
{
	class SHARD_API VariableStatementSyntax : public StatementSyntax
	{
	public:
		TypeSyntax* Type = nullptr;
		SyntaxToken IdentifierToken;
		SyntaxToken AssignToken;
		ExpressionSyntax* Expression = nullptr;

		inline VariableStatementSyntax(TypeSyntax* type, SyntaxToken name, SyntaxToken assignOp, ExpressionSyntax* expression, SyntaxNode *const parent)
			: StatementSyntax(SyntaxKind::VariableStatement, parent), Type(type), IdentifierToken(name), AssignToken(assignOp), Expression(expression) { }

		inline VariableStatementSyntax(const VariableStatementSyntax& other) = delete;

		inline virtual ~VariableStatementSyntax()
		{
			if (Type != nullptr)
			{
				Type->~TypeSyntax();
				delete Type;
			}

			if (Expression != nullptr)
			{
				Expression->~ExpressionSyntax();
				delete Expression;
			}
		}
	};
}
