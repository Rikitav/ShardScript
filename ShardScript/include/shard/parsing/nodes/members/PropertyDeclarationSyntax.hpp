#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API PropertyDeclarationSyntax final : public MemberDeclarationSyntax
	{
		SyntaxToken m_arrowToken;
		TypeSyntax* m_type = nullptr;
		SyntaxToken m_openBracketToken;
		AccessorDeclarationSyntax* m_getter = nullptr;
		AccessorDeclarationSyntax* m_setter = nullptr;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_assignToken;
		ExpressionSyntax* m_expression = nullptr;
		SyntaxToken m_semicolonToken;

	public:
		PropertyDeclarationSyntax(SyntaxNode* parent);
		virtual ~PropertyDeclarationSyntax() = default;

		SyntaxToken get_arrow() const;
		const TypeSyntax* get_type() const;
		SyntaxToken get_open_bracket() const;
		const AccessorDeclarationSyntax* get_getter() const;
		const AccessorDeclarationSyntax* get_setter() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_assign_token() const;
		const ExpressionSyntax* get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_arrow(const SyntaxToken& token);
		void set_type(TypeSyntax* type);
		void set_open_bracket(const SyntaxToken& token);
		void set_getter(AccessorDeclarationSyntax* getter);
		void set_setter(AccessorDeclarationSyntax* setter);
		void set_close_bracket(const SyntaxToken& token);
		void set_assign_token(const SyntaxToken& token);
		void set_expression(ExpressionSyntax* expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}