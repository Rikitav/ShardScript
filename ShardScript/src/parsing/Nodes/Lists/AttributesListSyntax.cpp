#include <shard/parsing/nodes/lists/AttributesListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

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
	return TextLocation(m_openBracketToken, m_closeBracketToken);
}

void AttributeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_attribute(this);
}

AttributesListSyntax::AttributesListSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::AttributesList, parent) { }

gmt::Span<const gmt::Ref<AttributeSyntax>> AttributesListSyntax::get_attributes() const
{
	return m_attributes;
}

SyntaxToken AttributesListSyntax::get_open_token() const
{
	return m_openToken;
}

SyntaxToken AttributesListSyntax::get_close_token() const
{
	return m_closeToken;
}

void AttributesListSyntax::set_attributes(gmt::Span<gmt::Ref<AttributeSyntax>> attributes)
{
	m_attributes = attributes;
}

void AttributesListSyntax::set_open_token(const SyntaxToken& token)
{
	m_openToken = token;
}

void AttributesListSyntax::set_close_token(const SyntaxToken& token)
{
	m_closeToken = token;
}

TextLocation AttributesListSyntax::get_location() const
{
	if (!m_attributes.empty())
	{
		std::span<const gmt::Ref<AttributeSyntax>> attributes = m_attributes.get();
		return TextLocation(attributes.front().get()->get_location(), attributes.back().get()->get_location());
	}

	return m_openToken.get_location();
}

void AttributesListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_attributes_list(this);
}
