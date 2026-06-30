#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/StatementSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ConditionalClauseBaseSyntax : public KeywordStatementSyntax
	{
	public:
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;
		std::unique_ptr<ConditionalClauseBaseSyntax> NextStatement = nullptr;

		inline ConditionalClauseBaseSyntax(const SyntaxKind kind, SyntaxNode* parent) : KeywordStatementSyntax(kind, parent) { }
		inline ConditionalClauseBaseSyntax(const ConditionalClauseBaseSyntax& other) = delete;

		inline virtual ~ConditionalClauseBaseSyntax() = default;
	};

	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::unique_ptr<StatementSyntax> ConditionExpression = nullptr;

		inline ConditionalClauseSyntax(const SyntaxKind kind, SyntaxNode* parent)
			: ConditionalClauseBaseSyntax(kind, parent) { }
		
		inline ConditionalClauseSyntax(const ConditionalClauseSyntax& other) = delete;

		inline virtual ~ConditionalClauseSyntax() = default;
	};

	class SHARD_API IfStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline IfStatementSyntax(SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }
		inline IfStatementSyntax(const IfStatementSyntax& other) = delete;
		inline virtual ~IfStatementSyntax() = default;
	};

	class SHARD_API UnlessStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline UnlessStatementSyntax(SyntaxNode* parent) : ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }
		inline UnlessStatementSyntax(const UnlessStatementSyntax& other) = delete;
		inline virtual ~UnlessStatementSyntax() = default;
	};

	class SHARD_API ElseStatementSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		inline ElseStatementSyntax(SyntaxNode* parent) : ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }
		inline ElseStatementSyntax(const ElseStatementSyntax& other) = delete;
		inline virtual ~ElseStatementSyntax() = default;
	};
}
