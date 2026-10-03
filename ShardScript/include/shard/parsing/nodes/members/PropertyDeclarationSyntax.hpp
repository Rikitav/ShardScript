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
		SyntaxToken m_colonToken;
		gmt::Ref<TypeSyntax> m_type;
		SyntaxToken m_openBracketToken;
		gmt::Ref<AccessorDeclarationSyntax> m_getter;
		gmt::Ref<AccessorDeclarationSyntax> m_setter;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_assignToken;
		gmt::Ref<ExpressionSyntax> m_expression;
		SyntaxToken m_semicolonToken;

	public:
		PropertyDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~PropertyDeclarationSyntax() = default;

		SyntaxToken get_colon() const;
		gmt::Ref<const TypeSyntax> get_type() const;
		SyntaxToken get_open_bracket() const;
		gmt::Ref<const AccessorDeclarationSyntax> get_getter() const;
		gmt::Ref<const AccessorDeclarationSyntax> get_setter() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_assign_token() const;
		gmt::Ref<const ExpressionSyntax> get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_colon(const SyntaxToken& token);
		void set_type(gmt::Ref<TypeSyntax> type);
		void set_open_bracket(const SyntaxToken& token);
		void set_getter(gmt::Ref<AccessorDeclarationSyntax> getter);
		void set_setter(gmt::Ref<AccessorDeclarationSyntax> setter);
		void set_close_bracket(const SyntaxToken& token);
		void set_assign_token(const SyntaxToken& token);
		void set_expression(gmt::Ref<ExpressionSyntax> expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
