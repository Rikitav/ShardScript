#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API ExpressionStatementSyntax final : public StatementSyntax
	{
		gmt::Ref<ExpressionSyntax> m_expression;
		SyntaxToken m_semicolonToken;

	public:
		ExpressionStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~ExpressionStatementSyntax() = default;

		gmt::Ref<const ExpressionSyntax> get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_expression(gmt::Ref<ExpressionSyntax> expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
