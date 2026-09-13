#include <shard/parsing/nodes/AttributeSyntax.hpp>
#include <gmt/Arena.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

using namespace shard;

AttributeSyntax::AttributeSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::Attribute, parent) { }

void AttributeSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void AttributeSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

void AttributeSyntax::set_name(const SyntaxToken& token)
{
	m_nameToken = token;
}

void AttributeSyntax::set_open_curl(const SyntaxToken& token)
{
	m_openCurlToken = token;
}

void AttributeSyntax::set_close_curl(const SyntaxToken& token)
{
	m_closeCurlToken = token;
}

void AttributeSyntax::set_arguments(gmt::Span<SyntaxToken> arguments)
{
	m_arguments = arguments;
}

SyntaxToken AttributeSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken AttributeSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

SyntaxToken AttributeSyntax::get_name() const
{
	return m_nameToken;
}

SyntaxToken AttributeSyntax::get_open_curl() const
{
	return m_openCurlToken;
}

SyntaxToken AttributeSyntax::get_close_curl() const
{
	return m_closeCurlToken;
}

gmt::Span<const SyntaxToken> AttributeSyntax::get_arguments() const
{
	return m_arguments;
}

TextLocation AttributeSyntax::get_location() const
{
	return m_openBracketToken.get_location();
}

void AttributeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_attribute(this);
}
