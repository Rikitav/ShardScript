#include <shard/parsing/nodes/lists/BaseTypesListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BaseTypesListSyntax::BaseTypesListSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::BaseTypesList, parent) { }

std::span<TypeSyntax* const> BaseTypesListSyntax::get_types() const
{
	return m_types;
}

void BaseTypesListSyntax::set_types(std::span<TypeSyntax*> types)
{
	m_types = types;
}

TextLocation BaseTypesListSyntax::get_location() const
{
	if (!m_types.empty())
	{
		std::span<TypeSyntax* const> types = m_types;
		return TextLocation(types.front()->get_location(), types.back()->get_location());
	}

	return TextLocation();
}

void BaseTypesListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_base_types_list(this);
}