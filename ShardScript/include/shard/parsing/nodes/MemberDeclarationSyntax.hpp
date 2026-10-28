#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/lists/AttributesListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <gmt/Ref.hpp>
#include <span>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
		AttributesListSyntax* m_attributes = nullptr;
		//TypeParametersListSyntax* m_typeParameters = nullptr;
		//std::span<WhereClauseSyntax*> m_whereClauses;

		std::span<SyntaxToken> m_modifiers;
		SyntaxToken m_declareToken;
		SyntaxToken m_identifierToken;

	public:
		MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent);
		virtual ~MemberDeclarationSyntax() = default;

		void set_attributes(AttributesListSyntax* attributes);
		//void set_type_parameters(TypeParametersListSyntax*);
		//void set_where_clauses(std::span<WhereClauseSyntax*>);
	
		void set_modifiers(std::span<SyntaxToken> modifiers);
		void set_identifier(const SyntaxToken& identifier);
		void set_declare_token(const SyntaxToken& declare);

		AttributesListSyntax* get_attributes() const;
		//TypeParametersListSyntax* get_type_parameters();
		//std::span<WhereClauseSyntax*> get_where_clauses();

		std::span<const SyntaxToken> get_modifiers() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_declare_token() const;
	};
}