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
		gmt::Ref<ParametersListSyntax> m_parametersList;
		SyntaxToken m_arrowToken;
		gmt::Ref<TypeSyntax> m_type;
		SyntaxToken m_openBracketToken;
		gmt::Ref<AccessorDeclarationSyntax> m_getter;
		gmt::Ref<AccessorDeclarationSyntax> m_setter;
		SyntaxToken m_closeBracketToken;

	public:
		IndexatorDeclarationSyntax(gmt::Ref<SyntaxNode> parent);
		virtual ~IndexatorDeclarationSyntax() = default;

		gmt::Ref<const ParametersListSyntax> get_parameters_list() const;
		SyntaxToken get_arrow() const;
		gmt::Ref<const TypeSyntax> get_type() const;
		SyntaxToken get_open_bracket() const;
		gmt::Ref<const AccessorDeclarationSyntax> get_getter() const;
		gmt::Ref<const AccessorDeclarationSyntax> get_setter() const;
		SyntaxToken get_close_bracket() const;

		void set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList);
		void set_arrow(const SyntaxToken& token);
		void set_type(gmt::Ref<TypeSyntax> type);
		void set_open_bracket(const SyntaxToken& token);
		void set_getter(gmt::Ref<AccessorDeclarationSyntax> getter);
		void set_setter(gmt::Ref<AccessorDeclarationSyntax> setter);
		void set_close_bracket(const SyntaxToken& token);

		TextLocation get_location() const override;
		void accept(SyntaxVisitor& visitor) const override;
	};
}
