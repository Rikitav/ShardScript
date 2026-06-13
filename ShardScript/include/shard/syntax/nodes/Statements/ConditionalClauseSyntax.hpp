#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxToken.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/nodes/StatementsBlockSyntax.hpp>
#include <shard/syntax/nodes/StatementSyntax.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ConditionalClauseBaseSyntax : public KeywordStatementSyntax
	{
	public:
		std::unique_ptr<StatementsBlockSyntax> StatementsBlock = nullptr;
		std::unique_ptr<ConditionalClauseBaseSyntax> NextStatement = nullptr;

		inline ConditionalClauseBaseSyntax(const SyntaxKind kind, SyntaxNode *const parent) : KeywordStatementSyntax(kind, parent) { }
		inline ConditionalClauseBaseSyntax(const ConditionalClauseBaseSyntax& other) = delete;

		inline virtual ~ConditionalClauseBaseSyntax() = default;
	};

	class SHARD_API ConditionalClauseSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		SyntaxToken OpenCurlToken;
		SyntaxToken CloseCurlToken;
		std::unique_ptr<StatementSyntax> ConditionExpression = nullptr;

		inline ConditionalClauseSyntax(const SyntaxKind kind, SyntaxNode *const parent)
			: ConditionalClauseBaseSyntax(kind, parent) { }
		
		inline ConditionalClauseSyntax(const ConditionalClauseSyntax& other) = delete;

		inline virtual ~ConditionalClauseSyntax() = default;
	};

	class SHARD_API IfStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline IfStatementSyntax(SyntaxNode *const parent) : ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }
		inline IfStatementSyntax(const IfStatementSyntax& other) = delete;
		inline virtual ~IfStatementSyntax() = default;
	};

	class SHARD_API UnlessStatementSyntax : public ConditionalClauseSyntax
	{
	public:
		inline UnlessStatementSyntax(SyntaxNode *const parent) : ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }
		inline UnlessStatementSyntax(const UnlessStatementSyntax& other) = delete;
		inline virtual ~UnlessStatementSyntax() = default;
	};

	class SHARD_API ElseStatementSyntax : public ConditionalClauseBaseSyntax
	{
	public:
		inline ElseStatementSyntax(SyntaxNode *const parent) : ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }
		inline ElseStatementSyntax(const ElseStatementSyntax& other) = delete;
		inline virtual ~ElseStatementSyntax() = default;
	};
}
