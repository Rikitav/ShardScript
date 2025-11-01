#pragma once
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class ForStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken FirstSemicolon;
		SyntaxToken SecondSemicolon;
		SyntaxToken CloseCurlToken;

		StatementSyntax* InitializerStatement = nullptr;
		ExpressionSyntax* ConditionExpression = nullptr;
		StatementSyntax* AfterRepeatStatement = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline ForStatementSyntax(const SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::ForStatement, parent) { }

		inline virtual ~ForStatementSyntax()
		{
			if (InitializerStatement != nullptr)
			{
				InitializerStatement->~StatementSyntax();
				delete InitializerStatement;
			}

			if (ConditionExpression != nullptr)
			{
				ConditionExpression->~ExpressionSyntax();
				delete ConditionExpression;
			}

			if (AfterRepeatStatement != nullptr)
			{
				AfterRepeatStatement->~StatementSyntax();
				delete AfterRepeatStatement;
			}

			if (StatementsBlock != nullptr)
			{
				StatementsBlock->~StatementsBlockSyntax();
				delete StatementsBlock;
			}
		}
	};
}
