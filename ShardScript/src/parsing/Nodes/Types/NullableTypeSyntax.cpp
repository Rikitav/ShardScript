#include <shard/parsing/nodes/types/NullableTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

#include <format>

using namespace shard;

NullableTypeSyntax::NullableTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::NullableType, parent) { }

gmt::Ref<const TypeSyntax> NullableTypeSyntax::get_underlaying_type() const
{
	return m_underlayingType;
}

SyntaxToken NullableTypeSyntax::get_question_token() const
{
	return m_questionToken;
}

void NullableTypeSyntax::set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType)
{
	m_underlayingType = underlayingType;
}

void NullableTypeSyntax::set_question_token(const SyntaxToken& token)
{
	m_questionToken = token;
}

std::wstring NullableTypeSyntax::get_qualifier() const
{
	if (m_underlayingType.is_null())
		return L"";

	return std::format(L"{}?", m_underlayingType.get()->get_qualifier());
}

TextLocation NullableTypeSyntax::get_location() const
{
	if (!m_underlayingType.is_null())
		return m_underlayingType.get()->get_location();

	return m_questionToken.get_location();
}

void NullableTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_nullable_type(this);
}
