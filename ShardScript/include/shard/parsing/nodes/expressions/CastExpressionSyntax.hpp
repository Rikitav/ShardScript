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
		ExpressionSyntax* m_expression = nullptr;
		TypeSyntax* m_targetType = nullptr;

	public:
		CastExpressionSyntax(SyntaxNode* parent);
		virtual ~CastExpressionSyntax() = default;

		SyntaxToken get_operator_token() const;
		const ExpressionSyntax* get_expression() const;
		const TypeSyntax* get_target_type() const;

		void set_operator_token(const SyntaxToken& token);
		void set_expression(ExpressionSyntax* expression);
		void set_target_type(TypeSyntax* targetType);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}