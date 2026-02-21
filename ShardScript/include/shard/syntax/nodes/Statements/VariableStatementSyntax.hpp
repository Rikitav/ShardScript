#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
