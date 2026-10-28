#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API FieldDeclarationSyntax final : public MemberDeclarationSyntax
	{
		SyntaxToken m_colonToken;
		TypeSyntax* m_type = nullptr;
		SyntaxToken m_assignToken;
		ExpressionSyntax* m_expression = nullptr;
		SyntaxToken m_semicolonToken;

	public:
		FieldDeclarationSyntax(SyntaxNode* parent);
		virtual ~FieldDeclarationSyntax() = default;

		SyntaxToken get_colon() const;
		const TypeSyntax* get_type() const;
		SyntaxToken get_assign_token() const;
		const ExpressionSyntax* get_expression() const;
		SyntaxToken get_semicolon() const;

		void set_colon(const SyntaxToken& token);
		void set_type(TypeSyntax* type);
		void set_assign_token(const SyntaxToken& token);
		void set_expression(ExpressionSyntax* expression);
		void set_semicolon(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}