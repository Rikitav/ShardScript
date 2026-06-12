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
	class SHARD_API ForEachStatementSyntax : public KeywordStatementSyntax
	{
	public:
		//SyntaxToken OpenCurlToken;
		//SyntaxToken CloseCurlToken;
		SyntaxToken IdentifierToken;
		SyntaxToken InKeywordToken;
		
		ExpressionSyntax* RangeExpression = nullptr;
		StatementsBlockSyntax* StatementsBlock = nullptr;

		inline ForEachStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::ForEachStatement, parent) { }

		inline ForEachStatementSyntax(const ForEachStatementSyntax& other) = delete;

		inline virtual ~ForEachStatementSyntax()
		{
			if (RangeExpression != nullptr)
			{
				RangeExpression->~ExpressionSyntax();
				delete RangeExpression;
			}

			if (StatementsBlock != nullptr)
			{
				StatementsBlock->~StatementsBlockSyntax();
				delete StatementsBlock;
			}
		}
	};
}
