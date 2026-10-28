#include <shard/parsing/nodes/members/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ConstructorDeclarationSyntax::ConstructorDeclarationSyntax(SyntaxNode* parent)
	: MemberDeclarationSyntax(SyntaxKind::ConstructorDeclaration, parent) { }

const ParametersListSyntax* ConstructorDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

const BlockSyntax* ConstructorDeclarationSyntax::get_body() const
{
	return m_body;
}

SyntaxToken ConstructorDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ConstructorDeclarationSyntax::set_parameters_list(ParametersListSyntax* parametersList)
{
	m_parametersList = parametersList;
}

void ConstructorDeclarationSyntax::set_body(BlockSyntax* body)
{
	m_body = body;
}

void ConstructorDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ConstructorDeclarationSyntax::get_location() const
{
	TextLocation location = get_declare_token().get_location();

	if (!m_semicolonToken.is_missing())
		return TextLocation(location, m_semicolonToken.get_location());

	if (!(m_body == nullptr))
		return TextLocation(location, m_body->get_location());

	if (!(m_parametersList == nullptr))
		return TextLocation(location, m_parametersList->get_location());

	return location;
}

void ConstructorDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_constructor_declaration(this);
}