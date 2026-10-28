#include <shard/parsing/nodes/lists/ParametersListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ParameterSyntax::ParameterSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::Parameter, parent) { }

const TypeSyntax* ParameterSyntax::get_type() const
{
	return m_type;
}

SyntaxToken ParameterSyntax::get_identifier() const
{
	return m_identifierToken;
}

void ParameterSyntax::set_type(TypeSyntax* type)
{
	m_type = type;
}

void ParameterSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

TextLocation ParameterSyntax::get_location() const
{
	if (!(m_type == nullptr))
		return TextLocation(m_identifierToken, m_type->get_location());

	return m_identifierToken.get_location();
}

void ParameterSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_parameter(this);
}

ParametersListSyntax::ParametersListSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::ParametersList, parent) { }

std::span<ParameterSyntax* const> ParametersListSyntax::get_parameters() const
{
	return m_parameters;
}

SyntaxToken ParametersListSyntax::get_open_token() const
{
	return m_openToken;
}

SyntaxToken ParametersListSyntax::get_close_token() const
{
	return m_closeToken;
}

void ParametersListSyntax::set_parameters(std::span<ParameterSyntax*> parameters)
{
	m_parameters = parameters;
}

void ParametersListSyntax::set_open_token(const SyntaxToken& token)
{
	m_openToken = token;
}

void ParametersListSyntax::set_close_token(const SyntaxToken& token)
{
	m_closeToken = token;
}

TextLocation ParametersListSyntax::get_location() const
{
	return TextLocation(m_openToken, m_closeToken);
}

void ParametersListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_parameters_list(this);
}