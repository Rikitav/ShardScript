#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>

#include <format>

using namespace shard;

NullableTypeSyntax::NullableTypeSyntax(SyntaxNode* parent)
	: TypeSyntax(SyntaxKind::NullableType, parent) { }

const TypeSyntax* NullableTypeSyntax::get_underlaying_type() const
{
	return m_underlayingType;
}

SyntaxToken NullableTypeSyntax::get_question_token() const
{
	return m_questionToken;
}

void NullableTypeSyntax::set_underlaying_type(TypeSyntax* underlayingType)
{
	m_underlayingType = underlayingType;
}

void NullableTypeSyntax::set_question_token(const SyntaxToken& token)
{
	m_questionToken = token;
}

gmt::wstring NullableTypeSyntax::get_qualifier() const
{
	if (m_underlayingType == nullptr)
		return L"";

	return gmt::wstring(std::format(L"{}?", m_underlayingType->get_qualifier()));
}

TextLocation NullableTypeSyntax::get_location() const
{
	if (!(m_underlayingType == nullptr))
		return TextLocation(m_underlayingType->get_location(), m_questionToken);

	return m_questionToken.get_location();
}

void NullableTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_nullable_type(this);
}
