#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <shard/syntax/nodes/StatementSyntax.h>
#include <shard/syntax/SyntaxNode.h>

namespace shard::syntax::nodes
{
	class ConditionalClauseBaseSyntax : public KeywordStatementSyntax
	{
	public:
		StatementsBlockSyntax* StatementsBlock = nullptr;
		ConditionalClauseBaseSyntax* NextStatement = nullptr;

		inline ConditionalClauseBaseSyntax(const SyntaxKind kind, const SyntaxNode* parent) : KeywordStatementSyntax(kind, parent) { }
		inline ConditionalClauseBaseSyntax(const ConditionalClauseBaseSyntax& other) : KeywordStatementSyntax(other), StatementsBlock(other.StatementsBlock), NextStatement(other.NextStatement) { }

		inline virtual ~ConditionalClauseBaseSyntax()
		{
			if (StatementsBlock != nullptr)
			{
				StatementsBlock->~StatementsBlockSyntax();
				delete StatementsBlock;
			}

			if (NextStatement != nullptr)
			{
				NextStatement->~ConditionalClauseBaseSyntax();
				delete NextStatement;
			}
		}
	};

	class ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		StatementSyntax* ConditionExpression = nullptr;

		inline ConditionalClauseSyntax(const SyntaxKind kind, const SyntaxNode* parent) : ConditionalClauseBaseSyntax(kind, parent) { }
		inline ConditionalClauseSyntax(const ConditionalClauseSyntax& other) : ConditionalClauseBaseSyntax(other), OpenCurlToken(other.OpenCurlToken), CloseCurlToken(other.CloseCurlToken), ConditionExpression(other.ConditionExpression) { }

		inline virtual ~ConditionalClauseSyntax()
		{
			if (ConditionExpression != nullptr)
			{
				ConditionExpression->~StatementSyntax();
				delete ConditionExpression;
			}
		}
	};

	class IfStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline IfStatementSyntax(const SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }
		inline IfStatementSyntax(const IfStatementSyntax& other) : ConditionalClauseSyntax(other) { }
		inline virtual ~IfStatementSyntax() { }
	};

	class UnlessStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline UnlessStatementSyntax(const SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }
		inline UnlessStatementSyntax(const UnlessStatementSyntax& other) : ConditionalClauseSyntax(other) { }
		inline virtual ~UnlessStatementSyntax() { }
	};

	class ElseSatetmentSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		inline ElseSatetmentSyntax(const SyntaxNode* parent) : ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }
		inline ElseSatetmentSyntax(const ElseSatetmentSyntax& other) : ConditionalClauseBaseSyntax(other) { }
		inline virtual ~ElseSatetmentSyntax() { }
	};
}
