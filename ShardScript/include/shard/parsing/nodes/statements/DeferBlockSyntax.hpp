#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API DeferBlockSyntax final : public StatementSyntax
	{
		SyntaxToken m_deferKeywordToken;
		gmt::Ref<StatementsBlockSyntax> m_block;

	public:
		DeferBlockSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~DeferBlockSyntax() = default;

		SyntaxToken get_defer_keyword() const;
		gmt::Ref<const StatementsBlockSyntax> get_block() const;

		void set_defer_keyword(const SyntaxToken& token);
		void set_block(gmt::Ref<StatementsBlockSyntax> block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
