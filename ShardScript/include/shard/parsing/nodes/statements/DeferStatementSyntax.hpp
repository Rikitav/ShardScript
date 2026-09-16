#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API DeferStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_deferKeywordToken;
		gmt::Ref<ExpressionSyntax> m_expression;
		SyntaxToken m_semicolonToken;

	public:
		DeferStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~DeferStatementSyntax() = default;

		SyntaxToken get_defer_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_defer_keyword(const SyntaxToken& token);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
