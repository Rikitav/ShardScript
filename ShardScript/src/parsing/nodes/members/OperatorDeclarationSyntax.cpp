#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

OperatorDeclarationSyntax::OperatorDeclarationSyntax(SyntaxNode* parent)
	: MemberDeclarationSyntax(SyntaxKind::OperatorDeclaration, parent) { }

SyntaxToken OperatorDeclarationSyntax::get_operator_token() const
{
	return m_operatorToken;
}

const ParametersListSyntax* OperatorDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

const TypeSyntax* OperatorDeclarationSyntax::get_return_type() const
{
	return m_returnType;
}

const BlockSyntax* OperatorDeclarationSyntax::get_body() const
{
	return m_body;
}

SyntaxToken OperatorDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void OperatorDeclarationSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void OperatorDeclarationSyntax::set_parameters_list(ParametersListSyntax* parametersList)
{
	m_parametersList = parametersList;
}

void OperatorDeclarationSyntax::set_return_type(TypeSyntax* returnType)
{
	m_returnType = returnType;
}

void OperatorDeclarationSyntax::set_body(BlockSyntax* body)
{
	m_body = body;
}

void OperatorDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation OperatorDeclarationSyntax::get_location() const
{
	TextLocation location = m_operatorToken.get_location();

	if (!m_semicolonToken.is_missing())
		return TextLocation(location, m_semicolonToken.get_location());

	if (!(m_body == nullptr))
		return TextLocation(location, m_body->get_location());

	if (!(m_returnType == nullptr))
		return TextLocation(location, m_returnType->get_location());

	return location;
}

void OperatorDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_operator_declaration(this);
}