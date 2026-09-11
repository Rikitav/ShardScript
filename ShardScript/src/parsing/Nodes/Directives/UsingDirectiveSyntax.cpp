#include <shard/parsing/nodes/Directives/UsingDirectiveSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <iostream>
#include <sstream>

using namespace shard;

UsingDirectiveSyntax::UsingDirectiveSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::UsingDirective, parent) {}

std::span<const SyntaxToken> UsingDirectiveSyntax::get_qualifier() const
{
	return std::span<const SyntaxToken>(m_qualifier.data(), m_qualifier.data() + m_qualifier.size());
}

string_t UsingDirectiveSyntax::get_qualifier_string() const
{
	UsingDirectiveSyntax* self = const_cast<UsingDirectiveSyntax*>(this);
	if (!m_qualifierStringCache.empty() && !m_changed)
		return m_qualifierStringCache.c_str();

	if (m_qualifier.empty())
		return L"";

	std::wstringstream qualifierString;
	for (std::size_t i = 0; i < m_qualifier.size() - 1; ++i)
		qualifierString << m_qualifier.at(i).get_lexeme() << L".";

	qualifierString << m_qualifier.back().get_lexeme();
	self->m_qualifierStringCache = qualifierString.str();
	self->m_changed = false;

	return m_qualifierStringCache.c_str();
}

SyntaxToken UsingDirectiveSyntax::get_using_keyword() const
{
	return m_usingKeyword;
}

SyntaxToken UsingDirectiveSyntax::get_semicolon() const
{
	return m_semicolon;
}

void UsingDirectiveSyntax::add_qulifier(const SyntaxToken& token)
{
	m_qualifier.push_back(token);
	m_changed = true;
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
