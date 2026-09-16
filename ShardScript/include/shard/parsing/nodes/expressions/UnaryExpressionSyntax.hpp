#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API UnaryExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_operatorToken;
		gmt::Ref<ExpressionSyntax> m_operand;
		bool m_isPostfix;

	public:
		UnaryExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~UnaryExpressionSyntax() = default;

		SyntaxToken get_operator_token() const;
		gmt::Ref<const ExpressionSyntax> get_operand() const;
		bool get_is_postfix() const;

		void set_operator_token(const SyntaxToken& token);
		void set_operand(gmt::Ref<ExpressionSyntax> operand);
		void set_is_postfix(bool isPostfix);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
