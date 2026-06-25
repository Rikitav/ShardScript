#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ForInStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken InKeywordToken;

		TypeSymbol* RangeType = nullptr;
		bool IsArrayRange = false;

		std::unique_ptr<ExpressionSyntax> RangeExpression = nullptr;
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;

		inline ForInStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::ForInStatement, parent) { }

		inline ForInStatementSyntax(const ForInStatementSyntax& other) = delete;

		inline virtual ~ForInStatementSyntax() = default;
	};
}
