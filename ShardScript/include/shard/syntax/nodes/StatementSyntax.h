#pragma once
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard::syntax::nodes
{
	class StatementSyntax : public SyntaxNode
	{
	public:
		SyntaxToken SemicolonToken;

		inline StatementSyntax(const SyntaxKind kind, const SyntaxNode* parent) : SyntaxNode(kind, parent)
		{
			/*
			if (kind < SyntaxKind::Statement || kind > SyntaxKind::ElseStatement)
				throw std::runtime_error("StatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline StatementSyntax(const StatementSyntax& other)
			: SyntaxNode(other), SemicolonToken(other.SemicolonToken) { }

		inline virtual ~StatementSyntax()
		{

		}
	};

	class KeywordStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken KeywordToken;

		inline KeywordStatementSyntax(const SyntaxKind kind, const SyntaxNode* parent) : StatementSyntax(kind, parent)
		{
			/*
			if (kind < SyntaxKind::KeywordStatement || kind > SyntaxKind::ElseStatement)
				throw std::runtime_error("KeywordStatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline KeywordStatementSyntax(const KeywordStatementSyntax& other)
			: StatementSyntax(other), KeywordToken(other.KeywordToken) { }

		inline virtual ~KeywordStatementSyntax()
		{

		}
	};
}
