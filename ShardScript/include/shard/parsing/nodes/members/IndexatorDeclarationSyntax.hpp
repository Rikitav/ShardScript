#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>

#include <gmt/Ref.hpp>

namespace shard
{
	class SHARD_API IndexatorDeclarationSyntax final : public MemberDeclarationSyntax
	{
		ParametersListSyntax* m_parametersList = nullptr;
		SyntaxToken m_arrowToken;
		TypeSyntax* m_type = nullptr;
		SyntaxToken m_openBracketToken;
		AccessorDeclarationSyntax* m_getter = nullptr;
		AccessorDeclarationSyntax* m_setter = nullptr;
		SyntaxToken m_closeBracketToken;

	public:
		IndexatorDeclarationSyntax(SyntaxNode* parent);
		virtual ~IndexatorDeclarationSyntax() = default;

		const ParametersListSyntax* get_parameters_list() const;
		SyntaxToken get_arrow() const;
		const TypeSyntax* get_type() const;
		SyntaxToken get_open_bracket() const;
		const AccessorDeclarationSyntax* get_getter() const;
		const AccessorDeclarationSyntax* get_setter() const;
		SyntaxToken get_close_bracket() const;

		void set_parameters_list(ParametersListSyntax* parametersList);
		void set_arrow(const SyntaxToken& token);
		void set_type(TypeSyntax* type);
		void set_open_bracket(const SyntaxToken& token);
		void set_getter(AccessorDeclarationSyntax* getter);
		void set_setter(AccessorDeclarationSyntax* setter);
		void set_close_bracket(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}