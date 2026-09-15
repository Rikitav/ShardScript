#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

TypeDeclarationSyntax::TypeDeclarationSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(kind, parent) { }

SyntaxToken TypeDeclarationSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken TypeDeclarationSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

SyntaxToken TypeDeclarationSyntax::get_base_type_colon() const
{
	return m_baseTypeColonToken;
}

SyntaxToken TypeDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

gmt::Ref<const BaseTypesListSyntax> TypeDeclarationSyntax::get_base_types() const
{
	return m_baseTypes;
}

gmt::Ref<const TypeParametersListSyntax> TypeDeclarationSyntax::get_type_parameters() const
{
	return m_typeParameters;
}

gmt::Ref<const WhereClausesListSyntax> TypeDeclarationSyntax::get_where_clauses() const
{
	return m_whereClauses;
}

gmt::Span<const gmt::Ref<MemberDeclarationSyntax>> TypeDeclarationSyntax::get_members() const
{
	return m_members;
}

void TypeDeclarationSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void TypeDeclarationSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

void TypeDeclarationSyntax::set_base_type_colon(const SyntaxToken& token)
{
	m_baseTypeColonToken = token;
}

void TypeDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

void TypeDeclarationSyntax::set_base_types(gmt::Ref<BaseTypesListSyntax> baseTypes)
{
	m_baseTypes = baseTypes;
}

void TypeDeclarationSyntax::set_type_parameters(gmt::Ref<TypeParametersListSyntax> typeParameters)
{
	m_typeParameters = typeParameters;
}

void TypeDeclarationSyntax::set_where_clauses(gmt::Ref<WhereClausesListSyntax> whereClauses)
{
	m_whereClauses = whereClauses;
}

void TypeDeclarationSyntax::set_members(gmt::Span<gmt::Ref<MemberDeclarationSyntax>> members)
{
	m_members = members;
}
