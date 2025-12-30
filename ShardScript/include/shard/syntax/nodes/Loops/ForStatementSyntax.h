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
	class SHARD_API ForStatementSyntax : public KeywordStatementSyntax
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

		inline ForStatementSyntax(const ForStatementSyntax&) = delete;

		inline virtual ~ForStatementSyntax()
		{
			if (InitializerStatement != nullptr)
				delete InitializerStatement;

			if (ConditionExpression != nullptr)
				delete ConditionExpression;

			if (AfterRepeatStatement != nullptr)
				delete AfterRepeatStatement;

			if (StatementsBlock != nullptr)
				delete StatementsBlock;
		}
	};
}
