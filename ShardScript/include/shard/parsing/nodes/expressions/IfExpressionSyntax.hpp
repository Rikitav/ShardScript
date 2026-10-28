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
		ExpressionSyntax* m_condition = nullptr;
		ExpressionSyntax* m_thenExpression = nullptr;
		SyntaxToken m_elseKeywordToken;
		ExpressionSyntax* m_elseExpression = nullptr;

	public:
		IfExpressionSyntax(SyntaxNode* parent);
		virtual ~IfExpressionSyntax() = default;

		SyntaxToken get_if_keyword() const;
		const ExpressionSyntax* get_condition() const;
		const ExpressionSyntax* get_then_expression() const;
		SyntaxToken get_else_keyword() const;
		const ExpressionSyntax* get_else_expression() const;

		void set_if_keyword(const SyntaxToken& token);
		void set_condition(ExpressionSyntax* condition);
		void set_then_expression(ExpressionSyntax* thenExpression);
		void set_else_keyword(const SyntaxToken& token);
		void set_else_expression(ExpressionSyntax* elseExpression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}