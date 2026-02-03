#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

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
