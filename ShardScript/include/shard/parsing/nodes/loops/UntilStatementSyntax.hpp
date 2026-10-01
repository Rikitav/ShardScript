#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API UntilStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_untilKeywordToken;
		gmt::Ref<ExpressionSyntax> m_condition;
		gmt::Ref<StatementsBlockSyntax> m_block;

	public:
		UntilStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~UntilStatementSyntax() = default;

		SyntaxToken get_until_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_condition() const;
		gmt::Ref<const StatementsBlockSyntax> get_block() const;

		void set_until_keyword(const SyntaxToken& token);
		void set_condition(gmt::Ref<ExpressionSyntax> condition);
		void set_block(gmt::Ref<StatementsBlockSyntax> block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
