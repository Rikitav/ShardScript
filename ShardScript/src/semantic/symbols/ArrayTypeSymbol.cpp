#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ArrayTypeSymbol::ArrayTypeSymbol(gmt::Ref<StructSymbol> underlayingType, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(gmt::intern(L"Array"), SyntaxKind::ArrayType, parent), m_underlayingType(underlayingType) { }

gmt::Ref<StructSymbol> ArrayTypeSymbol::get_underlaying_type() const
{
	return m_underlayingType;
}

std::size_t ArrayTypeSymbol::get_length() const
{
	return m_length;
}

void ArrayTypeSymbol::set_length(std::size_t length)
{
	m_length = length;
}
