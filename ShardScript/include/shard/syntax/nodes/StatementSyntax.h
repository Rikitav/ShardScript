#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>

namespace shard
{
	class SHARD_API StatementSyntax : public SyntaxNode
	{
	public:
		SyntaxToken SemicolonToken;

		inline StatementSyntax(const SyntaxKind kind, SyntaxNode *const parent) : SyntaxNode(kind, parent)
		{
			/*
			if (kind < SyntaxKind::Statement || kind > SyntaxKind::ElseStatement)
				throw std::std::runtime_error("StatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline StatementSyntax(const StatementSyntax& other) = delete;

		inline virtual ~StatementSyntax()
		{

		}
	};

	class SHARD_API KeywordStatementSyntax : public StatementSyntax
	{
	public:
		SyntaxToken KeywordToken;

		inline KeywordStatementSyntax(const SyntaxKind kind, SyntaxNode *const parent) : StatementSyntax(kind, parent)
		{
			/*
			if (kind < SyntaxKind::KeywordStatement || kind > SyntaxKind::ElseStatement)
				throw std::std::runtime_error("KeywordStatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline KeywordStatementSyntax(const KeywordStatementSyntax& other) = delete;

		inline virtual ~KeywordStatementSyntax()
		{

		}
	};
}
