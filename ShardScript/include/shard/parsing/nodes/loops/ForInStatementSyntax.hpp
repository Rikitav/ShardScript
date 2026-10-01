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
	class SHARD_API ForInStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_forKeywordToken;
		SyntaxToken m_identifierToken;
		SyntaxToken m_inKeywordToken;
		gmt::Ref<ExpressionSyntax> m_rangeExpression;
		gmt::Ref<StatementsBlockSyntax> m_block;

	public:
		ForInStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ForInStatementSyntax() = default;

		SyntaxToken get_for_keyword() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_in_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_range_expression() const;
		gmt::Ref<const StatementsBlockSyntax> get_block() const;

		void set_for_keyword(const SyntaxToken& token);
		void set_identifier(const SyntaxToken& token);
		void set_in_keyword(const SyntaxToken& token);
		void set_range_expression(gmt::Ref<ExpressionSyntax> rangeExpression);
		void set_block(gmt::Ref<StatementsBlockSyntax> block);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
