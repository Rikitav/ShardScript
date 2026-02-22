#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

namespace shard
{
	class SHARD_API WhileStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		ExpressionSyntax* ConditionExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline WhileStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::WhileStatement, parent) { }

		inline WhileStatementSyntax(const WhileStatementSyntax&) = delete;

		inline virtual ~WhileStatementSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
