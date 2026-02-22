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
	class SHARD_API UntilStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		ExpressionSyntax* ConditionExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline UntilStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::UntilStatement, parent) { }

		inline UntilStatementSyntax(const UntilStatementSyntax&) = delete;

		inline virtual ~UntilStatementSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
