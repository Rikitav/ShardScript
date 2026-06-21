#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ExpressionStatementSyntax : public StatementSyntax
	{
	public:
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;

		inline ExpressionStatementSyntax(SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }
		inline ExpressionStatementSyntax(std::unique_ptr<ExpressionSyntax> expression, SyntaxNode* parent) : StatementSyntax(SyntaxKind::ExpressionStatement, parent), Expression(std::move(expression)) { }
		inline ExpressionStatementSyntax(const ExpressionStatementSyntax& other) = delete;

		inline virtual ~ExpressionStatementSyntax() = default;
	};
}
