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
		gmt::Ref<StatementsBlockSyntax> m_block;
		gmt::Ref<ConditionalClauseBaseSyntax> m_next;

	public:
		ConditionalClauseBaseSyntax(SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~ConditionalClauseBaseSyntax() = default;

		gmt::Ref<const StatementsBlockSyntax> get_block() const;
		gmt::Ref<const ConditionalClauseBaseSyntax> get_next() const;

		void set_block(gmt::Ref<StatementsBlockSyntax> block);
		void set_next(gmt::Ref<ConditionalClauseBaseSyntax> next);
	};
}
