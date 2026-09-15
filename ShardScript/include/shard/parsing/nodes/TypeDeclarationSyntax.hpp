#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/parsing/nodes/TypeSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/lists/BaseTypesListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API TypeDeclarationSyntax : public MemberDeclarationSyntax
	{
		gmt::Ref<BaseTypesListSyntax> m_baseTypes;
		gmt::Ref<TypeParametersListSyntax> m_typeParameters;
		gmt::Ref<WhereClausesListSyntax> m_whereClauses;
		gmt::Span<gmt::Ref<MemberDeclarationSyntax>> m_members;

		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_baseTypeColonToken;
		SyntaxToken m_semicolonToken;

	public:
		TypeDeclarationSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~TypeDeclarationSyntax() = default;

		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_base_type_colon() const;
		SyntaxToken get_semicolon() const;

		gmt::Ref<const BaseTypesListSyntax> get_base_types() const;
		gmt::Ref<const TypeParametersListSyntax> get_type_parameters() const;
		gmt::Ref<const WhereClausesListSyntax> get_where_clauses() const;
		gmt::Span<const gmt::Ref<MemberDeclarationSyntax>> get_members() const;

		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);
		void set_base_type_colon(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		void set_base_types(gmt::Ref<BaseTypesListSyntax> baseTypes);
		void set_type_parameters(gmt::Ref<TypeParametersListSyntax> typeParameters);
		void set_where_clauses(gmt::Ref<WhereClausesListSyntax> whereClauses);
		void set_members(gmt::Span<gmt::Ref<MemberDeclarationSyntax>> members);
	};
}
