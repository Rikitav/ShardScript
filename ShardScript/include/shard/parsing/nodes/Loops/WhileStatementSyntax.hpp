#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <memory>

namespace shard
{
	class SHARD_API WhileStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		std::unique_ptr<ExpressionSyntax> ConditionExpression = nullptr;
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;

		inline WhileStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::WhileStatement, parent) { }

		inline WhileStatementSyntax(const WhileStatementSyntax&) = delete;

		inline virtual ~WhileStatementSyntax() = default;
	};
}
