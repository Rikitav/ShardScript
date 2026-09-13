#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

std::span<const gmt::Ref<TranslationUnitSyntax>> SyntaxTree::get_units() const
{
	return std::span(m_units.data(), m_units.data() + m_units.size());
}

void SyntaxTree::add_unit(gmt::Ref<TranslationUnitSyntax> unit)
{
	m_units.push_back(unit);
}
