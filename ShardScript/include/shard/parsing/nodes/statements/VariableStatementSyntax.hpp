#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API VariableStatementSyntax final : public StatementSyntax
	{
		SyntaxToken m_identifierToken;
		SyntaxToken m_assignToken;
		SyntaxToken m_semicolonToken;
		gmt::Ref<TypeSyntax> m_type;
		gmt::Ref<ExpressionSyntax> m_expression;

	public:
		VariableStatementSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~VariableStatementSyntax() = default;

		SyntaxToken get_identifier() const;
		SyntaxToken get_assign_token() const;
		SyntaxToken get_semicolon() const;
		gmt::Ref<const TypeSyntax> get_type() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;

		void set_identifier(const SyntaxToken& token);
		void set_assign_token(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);
		void set_type(gmt::Ref<TypeSyntax> type);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
