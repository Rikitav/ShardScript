#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

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
