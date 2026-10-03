#include <shard/parsing/nodes/members/IndexatorDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IndexatorDeclarationSyntax::IndexatorDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::IndexatorDeclaration, parent) { }

gmt::Ref<const ParametersListSyntax> IndexatorDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

SyntaxToken IndexatorDeclarationSyntax::get_arrow() const
{
	return m_arrowToken;
}

gmt::Ref<const TypeSyntax> IndexatorDeclarationSyntax::get_type() const
{
	return m_type;
}

SyntaxToken IndexatorDeclarationSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

gmt::Ref<const AccessorDeclarationSyntax> IndexatorDeclarationSyntax::get_getter() const
{
	return m_getter;
}

gmt::Ref<const AccessorDeclarationSyntax> IndexatorDeclarationSyntax::get_setter() const
{
	return m_setter;
}

SyntaxToken IndexatorDeclarationSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

void IndexatorDeclarationSyntax::set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList)
{
	m_parametersList = parametersList;
}

void IndexatorDeclarationSyntax::set_arrow(const SyntaxToken& token)
{
	m_arrowToken = token;
}

void IndexatorDeclarationSyntax::set_type(gmt::Ref<TypeSyntax> type)
{
	m_type = type;
}

void IndexatorDeclarationSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void IndexatorDeclarationSyntax::set_getter(gmt::Ref<AccessorDeclarationSyntax> getter)
{
	m_getter = getter;
}

void IndexatorDeclarationSyntax::set_setter(gmt::Ref<AccessorDeclarationSyntax> setter)
{
	m_setter = setter;
}

void IndexatorDeclarationSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

TextLocation IndexatorDeclarationSyntax::get_location() const
{
	SyntaxToken identifierToken = get_identifier();

	if (!m_closeBracketToken.is_missing())
		return TextLocation(identifierToken, m_closeBracketToken.get_location());

	if (!m_type.is_null())
		return TextLocation(identifierToken, m_type.as_ptr()->get_location());

	if (!m_parametersList.is_null())
		return TextLocation(identifierToken, m_parametersList.as_ptr()->get_location());

	return identifierToken.get_location();
}

void IndexatorDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_indexator_declaration(this);
}
