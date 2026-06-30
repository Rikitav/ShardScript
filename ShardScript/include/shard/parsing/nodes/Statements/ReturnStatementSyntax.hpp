#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ReturnStatementSyntax : public KeywordStatementSyntax
	{
	public:
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;

		inline ReturnStatementSyntax(SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ReturnStatement, parent) { }
		inline ReturnStatementSyntax(const ReturnStatementSyntax& other) = delete;

		inline virtual ~ReturnStatementSyntax() = default;
	};
}
