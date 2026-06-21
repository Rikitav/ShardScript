#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class SHARD_API VariableStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken AssignToken;

		std::unique_ptr<TypeSyntax> Type = nullptr;
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;

		inline VariableStatementSyntax(std::unique_ptr<TypeSyntax> type, SyntaxToken name, SyntaxToken assignOp, std::unique_ptr<ExpressionSyntax> expression, SyntaxNode* parent)
			: StatementSyntax(SyntaxKind::VariableStatement, parent), Type(std::move(type)), IdentifierToken(name), AssignToken(assignOp), Expression(std::move(expression)) { }

		inline VariableStatementSyntax(const VariableStatementSyntax& other) = delete;

		inline virtual ~VariableStatementSyntax() = default;
	};
}
