#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ConditionalClauseBaseSyntax : public StatementSyntax
	{
		StatementsBlockSyntax* m_block = nullptr;
		ConditionalClauseBaseSyntax* m_next = nullptr;

	public:
		ConditionalClauseBaseSyntax(SyntaxKind kind, SyntaxNode* parent);
		virtual ~ConditionalClauseBaseSyntax() = default;

		const StatementsBlockSyntax* get_block() const;
		const ConditionalClauseBaseSyntax* get_next() const;

		void set_block(StatementsBlockSyntax* block);
		void set_next(ConditionalClauseBaseSyntax* next);
	};
}