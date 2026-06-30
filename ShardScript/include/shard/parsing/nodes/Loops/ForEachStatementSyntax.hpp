#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ForEachStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken IdentifierToken;
		SyntaxToken InKeywordToken;

		TypeSymbol* RangeType = nullptr;
		bool IsArrayRange = false;

		std::unique_ptr<ExpressionSyntax> RangeExpression = nullptr;
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;

		inline ForEachStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::ForEachStatement, parent) { }

		inline ForEachStatementSyntax(const ForEachStatementSyntax& other) = delete;

		inline virtual ~ForEachStatementSyntax() = default;
	};
}
