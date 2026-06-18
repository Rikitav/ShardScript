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
	class SHARD_API ForEachStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken InKeywordToken;
		
		std::unique_ptr<ExpressionSyntax> RangeExpression = nullptr;
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;

		inline ForEachStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::ForEachStatement, parent) { }

		inline ForEachStatementSyntax(const ForEachStatementSyntax& other) = delete;

		inline virtual ~ForEachStatementSyntax() = default;
	};
}
