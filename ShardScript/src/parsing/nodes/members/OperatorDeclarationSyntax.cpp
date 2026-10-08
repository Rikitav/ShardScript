#include <shard/parsing/nodes/members/OperatorDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

OperatorDeclarationSyntax::OperatorDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::OperatorDeclaration, parent) { }

SyntaxToken OperatorDeclarationSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ParametersListSyntax> OperatorDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

gmt::Ref<const TypeSyntax> OperatorDeclarationSyntax::get_return_type() const
{
	return m_returnType;
}

gmt::Ref<const BlockSyntax> OperatorDeclarationSyntax::get_body() const
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

void OperatorDeclarationSyntax::set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList)
{
	m_parametersList = parametersList;
}

void OperatorDeclarationSyntax::set_return_type(gmt::Ref<TypeSyntax> returnType)
{
	m_returnType = returnType;
}

void OperatorDeclarationSyntax::set_body(gmt::Ref<BlockSyntax> body)
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

	if (!m_body.is_null())
		return TextLocation(location, m_body.as_ptr()->get_location());

	if (!m_returnType.is_null())
		return TextLocation(location, m_returnType.as_ptr()->get_location());

	return location;
}

void OperatorDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_operator_declaration(this);
}
