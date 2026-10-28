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
		TypeSyntax* m_type = nullptr;
		ExpressionSyntax* m_expression = nullptr;

	public:
		VariableStatementSyntax(SyntaxNode* parent);
		virtual ~VariableStatementSyntax() = default;

		SyntaxToken get_identifier() const;
		SyntaxToken get_assign_token() const;
		SyntaxToken get_semicolon() const;
		const TypeSyntax* get_type() const;
		const ExpressionSyntax* get_expression() const;

		void set_identifier(const SyntaxToken& token);
		void set_assign_token(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);
		void set_type(TypeSyntax* type);
		void set_expression(ExpressionSyntax* expression);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}