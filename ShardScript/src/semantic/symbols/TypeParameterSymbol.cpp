#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

TypeParameterSymbol::TypeParameterSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: TypeParameterSymbol(gmt::intern(name), parent) { }

TypeParameterSymbol::TypeParameterSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(name, SyntaxKind::TypeParameter, parent)
{
	set_accesibility(ACS_PUBLIC);
}

const std::vector<gmt::Ref<StructSymbol>>& TypeParameterSymbol::get_constraints() const
{
	return m_constraints;
}

void TypeParameterSymbol::add_constraint(gmt::Ref<StructSymbol> constraint)
{
	m_constraints.push_back(constraint);
}

std::uint16_t TypeParameterSymbol::get_type_argument_index() const
{
	return m_typeArgumentIndex;
}

void TypeParameterSymbol::set_type_argument_index(std::uint16_t index)
{
	m_typeArgumentIndex = index;
}
