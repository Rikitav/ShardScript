#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/parsing/nodes/lists/AttributesListSyntax.hpp>
#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>

#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

namespace shard
{
	class SHARD_API MemberDeclarationSyntax : public SyntaxNode
	{
		gmt::Ref<AttributesListSyntax> m_attributes;
		//gmt::Ref<TypeParametersListSyntax> m_typeParameters;
		//gmt::Span<gmt::Ref<WhereClauseSyntax>> m_whereClauses;

		gmt::Span<SyntaxToken> m_modifiers;
		SyntaxToken m_declareToken;
		SyntaxToken m_identifierToken;

	public:
		MemberDeclarationSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent);
		virtual ~MemberDeclarationSyntax() = default;

		void set_attributes(gmt::Ref<AttributesListSyntax> attributes);
		//void set_type_parameters(gmt::Ref<TypeParametersListSyntax>);
		//void set_where_clauses(gmt::Span<gmt::Ref<WhereClauseSyntax>>);
	
		void set_modifiers(gmt::Span<SyntaxToken> modifiers);
		void set_identifier(const SyntaxToken& identifier);
		void set_declare_token(const SyntaxToken& declare);

		gmt::Ref<AttributesListSyntax> get_attributes() const;
		//gmt::Ref<TypeParametersListSyntax> get_type_parameters();
		//gmt::Span<gmt::Ref<WhereClauseSyntax>> get_where_clauses();

		gmt::Span<const SyntaxToken> get_modifiers() const;
		SyntaxToken get_identifier() const;
		SyntaxToken get_declare_token() const;
	};
}
