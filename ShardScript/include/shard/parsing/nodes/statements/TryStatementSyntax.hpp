#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API TryStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_tryKeywordToken;
		gmt::Ref<StatementsBlockSyntax> m_tryBlock;
		gmt::Span<gmt::Ref<CatchClauseSyntax>> m_catchClauses;

	public:
		TryStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~TryStatementSyntax() = default;

		SyntaxToken get_try_keyword() const;
		gmt::Ref<const StatementsBlockSyntax> get_try_block() const;
		gmt::Span<const gmt::Ref<CatchClauseSyntax>> get_catch_clauses() const;

		void set_try_keyword(const SyntaxToken& token);
		void set_try_block(gmt::Ref<StatementsBlockSyntax> tryBlock);
		void set_catch_clauses(gmt::Span<gmt::Ref<CatchClauseSyntax>> catchClauses);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
