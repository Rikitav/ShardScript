#include <shard/parsing/nodes/members/DelegateDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DelegateDeclarationSyntax::DelegateDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeDeclarationSyntax(SyntaxKind::DelegateDeclaration, parent) { }

gmt::Ref<const ParametersListSyntax> DelegateDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

gmt::Ref<const TypeSyntax> DelegateDeclarationSyntax::get_return_type() const
{
	return m_returnType;
}

SyntaxToken DelegateDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void DelegateDeclarationSyntax::set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList)
{
	m_parametersList = parametersList;
}

void DelegateDeclarationSyntax::set_return_type(gmt::Ref<TypeSyntax> returnType)
{
	m_returnType = returnType;
}

void DelegateDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation DelegateDeclarationSyntax::get_location() const
{
	TextLocation location = get_declare_token().get_location();

	if (!m_semicolonToken.is_missing())
		return TextLocation(location, m_semicolonToken.get_location());

	if (!get_identifier().is_missing())
		return TextLocation(location, get_identifier().get_location());

	return location;
}

void DelegateDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_delegate_declaration(this);
}
