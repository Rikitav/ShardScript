#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/SyntaxNode.h>

#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>

namespace shard
{
	class SHARD_API ConditionalClauseBaseSyntax : public KeywordStatementSyntax
	{
	public:
		StatementsBlockSyntax* StatementsBlock = nullptr;
		ConditionalClauseBaseSyntax* NextStatement = nullptr;

		inline ConditionalClauseBaseSyntax(const SyntaxKind kind, const SyntaxNode* parent) : KeywordStatementSyntax(kind, parent) { }
		inline ConditionalClauseBaseSyntax(const ConditionalClauseBaseSyntax& other) = delete;

		inline virtual ~ConditionalClauseBaseSyntax()
		{
			if (StatementsBlock != nullptr)
				delete StatementsBlock;

			if (NextStatement != nullptr)
				delete NextStatement;
		}
	};

	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		StatementSyntax* ConditionExpression = nullptr;

		inline ConditionalClauseSyntax(const SyntaxKind kind, const SyntaxNode* parent)
			: ConditionalClauseBaseSyntax(kind, parent) { }
		
		inline ConditionalClauseSyntax(const ConditionalClauseSyntax& other) = delete;

		inline virtual ~ConditionalClauseSyntax()
		{
			if (ConditionExpression != nullptr)
				delete ConditionExpression;
		}
	};

	class SHARD_API IfStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline IfStatementSyntax(const SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }
		inline IfStatementSyntax(const IfStatementSyntax& other) = delete;
		inline virtual ~IfStatementSyntax() { }
	};

	class SHARD_API UnlessStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline UnlessStatementSyntax(const SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }
		inline UnlessStatementSyntax(const UnlessStatementSyntax& other) = delete;
		inline virtual ~UnlessStatementSyntax() { }
	};

	class SHARD_API ElseStatementSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		inline ElseStatementSyntax(const SyntaxNode* parent) : ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }
		inline ElseStatementSyntax(const ElseStatementSyntax& other) = delete;
		inline virtual ~ElseStatementSyntax() { }
	};
}
