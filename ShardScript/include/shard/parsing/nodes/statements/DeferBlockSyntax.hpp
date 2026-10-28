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
		StatementsBlockSyntax* m_block = nullptr;

	public:
		DeferBlockSyntax(SyntaxNode* parent);
		virtual ~DeferBlockSyntax() = default;

		SyntaxToken get_defer_keyword() const;
		const StatementsBlockSyntax* get_block() const;

		void set_defer_keyword(const SyntaxToken& token);
		void set_block(StatementsBlockSyntax* block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}