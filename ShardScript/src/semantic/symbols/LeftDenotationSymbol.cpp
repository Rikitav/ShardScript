#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

LeftDenotationSymbol::LeftDenotationSymbol(StructSymbol* expectedType, SyntaxSymbol* parent)
	: SyntaxSymbol(gmt::intern(L""), SyntaxKind::Argument, parent), m_expectedType(expectedType) { }

StructSymbol* LeftDenotationSymbol::get_expected_type() const
{
	return m_expectedType;
}

void LeftDenotationSymbol::set_expected_type(StructSymbol* expectedType)
{
	m_expectedType = expectedType;
}