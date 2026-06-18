#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/nodes/TypeSyntax.hpp>

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

		inline CatchClauseSyntax(SyntaxNode* const parent)
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

		inline TryStatementSyntax(SyntaxNode *const parent)
			: KeywordStatementSyntax(SyntaxKind::TryStatement, parent) { }

		inline TryStatementSyntax(const TryStatementSyntax&) = delete;

		inline virtual ~TryStatementSyntax() = default;
	};
}
