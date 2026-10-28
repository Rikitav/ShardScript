#include <shard/parsing/nodes/lists/ArgumentsListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ArgumentSyntax::ArgumentSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::Argument, parent) { }

const ExpressionSyntax* ArgumentSyntax::get_expression() const
{
	return m_expression;
}

void ArgumentSyntax::set_expression(ExpressionSyntax* expression)
{
	m_expression = expression;
}

TextLocation ArgumentSyntax::get_location() const
{
	if (!(m_expression == nullptr))
		return m_expression->get_location();

	return TextLocation();
}

void ArgumentSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_argument(this);
}

ArgumentsListSyntax::ArgumentsListSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::ArgumentsList, parent) { }

std::span<ArgumentSyntax* const> ArgumentsListSyntax::get_arguments() const
{
	return m_arguments;
}

SyntaxToken ArgumentsListSyntax::get_open_token() const
{
	return m_openToken;
}

SyntaxToken ArgumentsListSyntax::get_close_token() const
{
	return m_closeToken;
}

void ArgumentsListSyntax::set_arguments(std::span<ArgumentSyntax*> arguments)
{
	m_arguments = arguments;
}

void ArgumentsListSyntax::set_open_token(const SyntaxToken& token)
{
	m_openToken = token;
}

void ArgumentsListSyntax::set_close_token(const SyntaxToken& token)
{
	m_closeToken = token;
}

TextLocation ArgumentsListSyntax::get_location() const
{
	return TextLocation(m_openToken, m_closeToken);
}

void ArgumentsListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_arguments_list(this);
}