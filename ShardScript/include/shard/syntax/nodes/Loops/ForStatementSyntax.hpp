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
	class SHARD_API ForStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken FirstSemicolon;
		SyntaxToken SecondSemicolon;
		SyntaxToken CloseCurlToken;

		std::unique_ptr<StatementSyntax> InitializerStatement = nullptr;
		std::unique_ptr<ExpressionSyntax> ConditionExpression = nullptr;
		std::unique_ptr<StatementSyntax> AfterRepeatStatement = nullptr;
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;

		inline ForStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::ForStatement, parent) { }

		inline ForStatementSyntax(const ForStatementSyntax&) = delete;

		inline virtual ~ForStatementSyntax() = default;
	};
}
