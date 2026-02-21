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

		inline ForStatementSyntax(SyntaxNode *const parent)
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
