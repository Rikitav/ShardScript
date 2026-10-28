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
#include <span>

namespace shard
{
	class SHARD_API TypeDeclarationSyntax : public MemberDeclarationSyntax
	{
		BaseTypesListSyntax* m_baseTypes = nullptr;
		TypeParametersListSyntax* m_typeParameters = nullptr;
		WhereClausesListSyntax* m_whereClauses = nullptr;
		std::span<MemberDeclarationSyntax*> m_members;

		SyntaxToken m_openBracketToken;
		SyntaxToken m_closeBracketToken;
		SyntaxToken m_baseTypeColonToken;
		SyntaxToken m_semicolonToken;

	public:
		TypeDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent);
		virtual ~TypeDeclarationSyntax() = default;

		SyntaxToken get_open_bracket() const;
		SyntaxToken get_close_bracket() const;
		SyntaxToken get_base_type_colon() const;
		SyntaxToken get_semicolon() const;

		const BaseTypesListSyntax* get_base_types() const;
		const TypeParametersListSyntax* get_type_parameters() const;
		const WhereClausesListSyntax* get_where_clauses() const;
		std::span<MemberDeclarationSyntax* const> get_members() const;

		void set_open_bracket(const SyntaxToken& token);
		void set_close_bracket(const SyntaxToken& token);
		void set_base_type_colon(const SyntaxToken& token);
		void set_semicolon(const SyntaxToken& token);

		void set_base_types(BaseTypesListSyntax* baseTypes);
		void set_type_parameters(TypeParametersListSyntax* typeParameters);
		void set_where_clauses(WhereClausesListSyntax* whereClauses);
		void set_members(std::span<MemberDeclarationSyntax*> members);
	};
}