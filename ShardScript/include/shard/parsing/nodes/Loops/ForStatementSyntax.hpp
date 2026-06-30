#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

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

		inline ForStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::ForStatement, parent) { }

		inline ForStatementSyntax(const ForStatementSyntax&) = delete;

		inline virtual ~ForStatementSyntax() = default;
	};
}
