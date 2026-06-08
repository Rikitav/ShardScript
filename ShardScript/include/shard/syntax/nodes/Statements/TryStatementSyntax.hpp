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

namespace shard
{
	class SHARD_API CatchClauseSyntax : public SyntaxNode
	{
	public:
		SyntaxToken CatchKeywordToken;
		SyntaxToken IdentifierToken;
		SyntaxToken ColonToken;
		TypeSyntax* ExceptionType = nullptr;
		StatementsBlockSyntax* Body = nullptr;

		inline CatchClauseSyntax(SyntaxNode* const parent)
			: SyntaxNode(SyntaxKind::CatchClause, parent) { }

		inline CatchClauseSyntax(const CatchClauseSyntax&) = delete;

		inline virtual ~CatchClauseSyntax()
		{
			if (ExceptionType != nullptr)
				delete ExceptionType;
			if (Body != nullptr)
				delete Body;
		}
	};

	class SHARD_API TryStatementSyntax : public KeywordStatementSyntax
	{
	public:
		SyntaxToken TryKeywordToken;
		StatementsBlockSyntax* TryBlock = nullptr;
		std::vector<CatchClauseSyntax*> CatchClauses;

		inline TryStatementSyntax(SyntaxNode* const parent)
			: KeywordStatementSyntax(SyntaxKind::TryStatement, parent) { }

		inline TryStatementSyntax(const TryStatementSyntax&) = delete;

		inline virtual ~TryStatementSyntax()
		{
			if (TryBlock != nullptr)
				delete TryBlock;
			for (auto clause : CatchClauses)
				delete clause;
		}
	};
}
