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
		ExpressionSyntax* m_condition = nullptr;
		StatementsBlockSyntax* m_block = nullptr;

	public:
		UntilStatementSyntax(SyntaxNode* parent);
		virtual ~UntilStatementSyntax() = default;

		SyntaxToken get_until_keyword() const;
		const ExpressionSyntax* get_condition() const;
		const StatementsBlockSyntax* get_block() const;

		void set_until_keyword(const SyntaxToken& token);
		void set_condition(ExpressionSyntax* condition);
		void set_block(StatementsBlockSyntax* block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}