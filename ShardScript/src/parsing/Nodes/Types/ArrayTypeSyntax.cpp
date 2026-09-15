#include <shard/parsing/nodes/types/ArrayTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

#include <format>

using namespace shard;

ArrayTypeSyntax::ArrayTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::ArrayType, parent) { }

gmt::Ref<const TypeSyntax> ArrayTypeSyntax::get_underlaying_type() const
{
	return m_underlayingType;
}

SyntaxToken ArrayTypeSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken ArrayTypeSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

void ArrayTypeSyntax::set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType)
{
	m_underlayingType = underlayingType;
}

void ArrayTypeSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void ArrayTypeSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

std::wstring ArrayTypeSyntax::get_qualifier() const
{
	if (m_underlayingType.is_null())
		return L"";

	return std::format(L"{}[]", m_underlayingType.get()->get_qualifier());
}

TextLocation ArrayTypeSyntax::get_location() const
{
	if (!m_underlayingType.is_null())
		return TextLocation(m_underlayingType.get()->get_location(), m_closeBracketToken);

	return m_openBracketToken.get_location();
}

void ArrayTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_array_type(this);
}
