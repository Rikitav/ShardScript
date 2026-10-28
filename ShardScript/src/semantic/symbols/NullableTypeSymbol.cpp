#include <shard/semantic/symbols/NullableTypeSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

NullableTypeSymbol::NullableTypeSymbol(StructSymbol* underlayingType, SyntaxSymbol* parent)
	: StructSymbol(gmt::intern(L"Nullable"), SyntaxKind::NullableType, parent), m_underlayingType(underlayingType) { }

StructSymbol* NullableTypeSymbol::get_underlaying_type() const
{
	return m_underlayingType;
}