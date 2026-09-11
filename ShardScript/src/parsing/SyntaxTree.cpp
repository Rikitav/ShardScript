#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

std::span<const TranslationUnitSyntax> SyntaxTree::get_units() const
{
	return std::span<const TranslationUnitSyntax>(m_units.data(), m_units.data() + m_units.size());
}

void SyntaxTree::add_unit(TranslationUnitSyntax& unit)
{
	m_units.push_back(unit);
}
