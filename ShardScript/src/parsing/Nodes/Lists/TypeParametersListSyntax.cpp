#include <shard/parsing/nodes/lists/TypeParametersListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

TypeParameterSyntax::TypeParameterSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::TypeParameter, parent) { }

SyntaxToken TypeParameterSyntax::get_identifier() const
{
	return m_identifierToken;
}

void TypeParameterSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

TextLocation TypeParameterSyntax::get_location() const
{
	return m_identifierToken.get_location();
}

void TypeParameterSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_type_parameter(this);
}

TypeParametersListSyntax::TypeParametersListSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::TypeParametersList, parent) { }

gmt::Span<const gmt::Ref<TypeParameterSyntax>> TypeParametersListSyntax::get_parameters() const
{
	return m_parameters;
}

SyntaxToken TypeParametersListSyntax::get_open_token() const
{
	return m_openToken;
}

SyntaxToken TypeParametersListSyntax::get_close_token() const
{
	return m_closeToken;
}

void TypeParametersListSyntax::set_parameters(gmt::Span<gmt::Ref<TypeParameterSyntax>> parameters)
{
	m_parameters = parameters;
}

void TypeParametersListSyntax::set_open_token(const SyntaxToken& token)
{
	m_openToken = token;
}

void TypeParametersListSyntax::set_close_token(const SyntaxToken& token)
{
	m_closeToken = token;
}

TextLocation TypeParametersListSyntax::get_location() const
{
	return m_openToken.get_location();
}

void TypeParametersListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_type_parameters_list(this);
}
