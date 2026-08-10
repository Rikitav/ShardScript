#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/Statements/SwitchCaseClauseSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class SHARD_API SwitchStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken SwitchKeywordToken;
		SyntaxToken OpenParenToken;
		SyntaxToken CloseParenToken;
		SyntaxToken OpenBraceToken;
		SyntaxToken CloseBraceToken;

		std::unique_ptr<ExpressionSyntax> Expression = nullptr;
		std::vector<std::unique_ptr<SwitchCaseClauseSyntax>> Clauses;

		inline SwitchStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::SwitchStatement, parent) { }

		inline SwitchStatementSyntax(const SwitchStatementSyntax&) = delete;

		inline virtual ~SwitchStatementSyntax() = default;
	};
}
