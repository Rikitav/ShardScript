#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API IfExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_ifKeywordToken;
		gmt::Ref<ExpressionSyntax> m_condition;
		gmt::Ref<ExpressionSyntax> m_thenExpression;
		SyntaxToken m_elseKeywordToken;
		gmt::Ref<ExpressionSyntax> m_elseExpression;

	public:
		IfExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~IfExpressionSyntax() = default;

		SyntaxToken get_if_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_condition() const;
		gmt::Ref<const ExpressionSyntax> get_then_expression() const;
		SyntaxToken get_else_keyword() const;
		gmt::Ref<const ExpressionSyntax> get_else_expression() const;

		void set_if_keyword(const SyntaxToken& token);
		void set_condition(gmt::Ref<ExpressionSyntax> condition);
		void set_then_expression(gmt::Ref<ExpressionSyntax> thenExpression);
		void set_else_keyword(const SyntaxToken& token);
		void set_else_expression(gmt::Ref<ExpressionSyntax> elseExpression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
