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
