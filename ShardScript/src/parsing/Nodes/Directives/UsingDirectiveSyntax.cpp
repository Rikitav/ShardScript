#include <shard/parsing/nodes/directives/UsingDirectiveSyntax.hpp>
#include <gmt/Arena.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <sstream>

using namespace shard;

UsingDirectiveSyntax::UsingDirectiveSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::UsingDirective, parent) {}

gmt::Span<const SyntaxToken> UsingDirectiveSyntax::get_qualifier() const
{
	return m_qualifier;
}

std::wstring UsingDirectiveSyntax::get_qualifier_string() const
{
	std::span<const SyntaxToken> qualifier = m_qualifier.get();
	if (qualifier.empty())
		return L"";

	std::wstringstream qualifierString;
	for (std::size_t i = 0; i < qualifier.size() - 1; ++i)
		qualifierString << qualifier[i].get_lexeme() << L".";

	qualifierString << qualifier.back().get_lexeme();
	return qualifierString.str();
}

SyntaxToken UsingDirectiveSyntax::get_using_keyword() const
{
	return m_usingKeyword;
}

SyntaxToken UsingDirectiveSyntax::get_semicolon() const
{
	return m_semicolon;
}

void UsingDirectiveSyntax::set_qualifier(gmt::Span<SyntaxToken> qualifier)
{
	m_qualifier = qualifier;
}

void UsingDirectiveSyntax::set_using_keyword(const SyntaxToken& token)
{
	m_usingKeyword = token;
}

void UsingDirectiveSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolon = token;
}

TextLocation UsingDirectiveSyntax::get_location() const
{
	return TextLocation(m_usingKeyword, m_semicolon);
}

void UsingDirectiveSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_using_directive(this);
}
