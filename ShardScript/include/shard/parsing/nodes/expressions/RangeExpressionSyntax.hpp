#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API RangeExpressionSyntax final : public ExpressionSyntax
	{
		SyntaxToken m_operatorToken;
		gmt::Ref<ExpressionSyntax> m_left;
		gmt::Ref<ExpressionSyntax> m_right;

	public:
		RangeExpressionSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~RangeExpressionSyntax() = default;

		SyntaxToken get_operator_token() const;
		gmt::Ref<const ExpressionSyntax> get_left() const;
		gmt::Ref<const ExpressionSyntax> get_right() const;

		bool is_inclusive() const;

		void set_operator_token(const SyntaxToken& token);
		void set_left(gmt::Ref<ExpressionSyntax> left);
		void set_right(gmt::Ref<ExpressionSyntax> right);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
