#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API CastExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_operatorToken;
		gmt::Ref<ExpressionSyntax> m_expression;
		gmt::Ref<TypeSyntax> m_targetType;

	public:
		CastExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~CastExpressionSyntax() = default;

		SyntaxToken get_operator_token() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;
		gmt::Ref<const TypeSyntax> get_target_type() const;

		void set_operator_token(const SyntaxToken& token);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);
		void set_target_type(gmt::Ref<TypeSyntax> targetType);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
