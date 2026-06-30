#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <vector>
#include <memory>

namespace shard
{
	class VariableSymbol;

	class SHARD_API CatchClauseSyntax : public SyntaxNode
	{
	public:
		SyntaxToken CatchKeywordToken;
		SyntaxToken OpenParenToken;
		SyntaxToken IdentifierToken;
		SyntaxToken ColonToken;
		SyntaxToken CloseParenToken;

		std::unique_ptr<TypeSyntax> ExceptionType = nullptr;
		std::unique_ptr<StatementsBlockSyntax> Body = nullptr;
		VariableSymbol* Symbol = nullptr;

		inline CatchClauseSyntax(SyntaxNode* parent)
			: SyntaxNode(SyntaxKind::CatchClause, parent) { }

		inline CatchClauseSyntax(const CatchClauseSyntax&) = delete;

		inline virtual ~CatchClauseSyntax() = default;
	};

	class SHARD_API TryStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken TryKeywordToken;
		std::unique_ptr<StatementsBlockSyntax> TryBlock = nullptr;
		std::vector<std::unique_ptr<CatchClauseSyntax>> CatchClauses;

		inline TryStatementSyntax(SyntaxNode* parent)
			: KeywordStatementSyntax(SyntaxKind::TryStatement, parent) { }

		inline TryStatementSyntax(const TryStatementSyntax&) = delete;

		inline virtual ~TryStatementSyntax() = default;
	};
}
