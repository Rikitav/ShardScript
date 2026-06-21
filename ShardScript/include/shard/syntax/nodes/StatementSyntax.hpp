#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>

namespace shard
{
	class SHARD_API StatementSyntax : public SyntaxNode
	{
	public:
		SyntaxToken SemicolonToken;

		inline StatementSyntax(const SyntaxKind kind, SyntaxNode* parent) : SyntaxNode(kind, parent)
		{
			/*
			if (kind < SyntaxKind::Statement || kind > SyntaxKind::ElseStatement)
				throw std::runtime_error("StatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
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

		inline KeywordStatementSyntax(const SyntaxKind kind, SyntaxNode* parent) : StatementSyntax(kind, parent)
		{
			/*
			if (kind < SyntaxKind::KeywordStatement || kind > SyntaxKind::ElseStatement)
				throw std::runtime_error("KeywordStatementSyntax kind out of range (" + std::to_string(static_cast<int>(kind)) + ")");
			*/
		}

		inline KeywordStatementSyntax(const KeywordStatementSyntax& other) = delete;

		inline virtual ~KeywordStatementSyntax()
		{

		}
	};
}
