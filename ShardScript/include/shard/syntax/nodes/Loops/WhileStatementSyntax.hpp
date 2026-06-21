#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

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
