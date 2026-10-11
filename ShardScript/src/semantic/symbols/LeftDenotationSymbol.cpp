#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

LeftDenotationSymbol::LeftDenotationSymbol(gmt::Ref<StructSymbol> expected_type, gmt::Ref<SyntaxSymbol> parent)
	: SyntaxSymbol(gmt::intern(L""), SyntaxKind::Argument, parent), m_expectedType(expected_type) { }

gmt::Ref<StructSymbol> LeftDenotationSymbol::get_expected_type() const
{
	return m_expectedType;
}

void LeftDenotationSymbol::set_expected_type(gmt::Ref<StructSymbol> expected_type)
{
	m_expectedType = expected_type;
}
