#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API TryStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_tryKeywordToken;
		StatementsBlockSyntax* m_tryBlock = nullptr;
		std::span<CatchClauseSyntax*> m_catchClauses;

	public:
		TryStatementSyntax(SyntaxNode* parent);
		virtual ~TryStatementSyntax() = default;

		SyntaxToken get_try_keyword() const;
		const StatementsBlockSyntax* get_try_block() const;
		std::span<CatchClauseSyntax* const> get_catch_clauses() const;

		void set_try_keyword(const SyntaxToken& token);
		void set_try_block(StatementsBlockSyntax* tryBlock);
		void set_catch_clauses(std::span<CatchClauseSyntax*> catchClauses);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}