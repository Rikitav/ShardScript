#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ThrowStatementSyntax : public KeywordStatementSyntax
	{
	public:
		std::unique_ptr<ExpressionSyntax> Expression = nullptr;

		inline ThrowStatementSyntax(SyntaxNode* parent) : KeywordStatementSyntax(SyntaxKind::ThrowStatement, parent) {}
		inline ThrowStatementSyntax(const ThrowStatementSyntax& other) = delete;

		inline virtual ~ThrowStatementSyntax() = default;
	};
}
