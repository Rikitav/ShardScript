#pragma once
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class UntilStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;

		ExpressionSyntax* ConditionExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline UntilStatementSyntax(const SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::UntilStatement, parent) { }

		inline virtual ~UntilStatementSyntax()
		{
			if (ConditionExpression != nullptr)
			{
				ConditionExpression->~ExpressionSyntax();
				delete ConditionExpression;
			}

			if (StatementsBlock != nullptr)
			{
				StatementsBlock->~StatementsBlockSyntax();
				delete StatementsBlock;
			}
		}
	};
}
