#include <shard/parsing/nodes/lists/BaseTypesListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BaseTypesListSyntax::BaseTypesListSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::BaseTypesList, parent) { }

gmt::Span<const gmt::Ref<TypeSyntax>> BaseTypesListSyntax::get_types() const
{
	return m_types;
}

void BaseTypesListSyntax::set_types(gmt::Span<gmt::Ref<TypeSyntax>> types)
{
	m_types = types;
}

TextLocation BaseTypesListSyntax::get_location() const
{
	if (!m_types.empty())
	{
		std::span<const gmt::Ref<TypeSyntax>> types = m_types.as_span();
		return TextLocation(types.front().as_ptr()->get_location(), types.back().as_ptr()->get_location());
	}

	return TextLocation();
}

void BaseTypesListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_base_types_list(this);
}
