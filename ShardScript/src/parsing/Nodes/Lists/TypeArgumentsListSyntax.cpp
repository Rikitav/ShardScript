#include <shard/parsing/nodes/lists/TypeArgumentsListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

TypeArgumentsListSyntax::TypeArgumentsListSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::TypeArgumentsList, parent) { }

gmt::Span<const gmt::Ref<TypeSyntax>> TypeArgumentsListSyntax::get_types() const
{
	return m_types;
}

SyntaxToken TypeArgumentsListSyntax::get_open_token() const
{
	return m_openToken;
}

SyntaxToken TypeArgumentsListSyntax::get_close_token() const
{
	return m_closeToken;
}

void TypeArgumentsListSyntax::set_types(gmt::Span<gmt::Ref<TypeSyntax>> types)
{
	m_types = types;
}

void TypeArgumentsListSyntax::set_open_token(const SyntaxToken& token)
{
	m_openToken = token;
}

void TypeArgumentsListSyntax::set_close_token(const SyntaxToken& token)
{
	m_closeToken = token;
}

TextLocation TypeArgumentsListSyntax::get_location() const
{
	return m_openToken.get_location();
}

void TypeArgumentsListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_type_arguments_list(this);
}
