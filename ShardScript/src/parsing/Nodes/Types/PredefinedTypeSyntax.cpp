#include <shard/parsing/nodes/types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

PredefinedTypeSyntax::PredefinedTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::PredefinedType, parent) { }

SyntaxToken PredefinedTypeSyntax::get_type_token() const
{
	return m_typeToken;
}

void PredefinedTypeSyntax::set_type_token(const SyntaxToken& token)
{
	m_typeToken = token;
}

std::wstring PredefinedTypeSyntax::get_qualifier() const
{
	return std::wstring(m_typeToken.get_lexeme());
}

TextLocation PredefinedTypeSyntax::get_location() const
{
	return m_typeToken.get_location();
}

void PredefinedTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_predefined_type(this);
}
