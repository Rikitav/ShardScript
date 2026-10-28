#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ParameterSymbol::ParameterSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: ParameterSymbol(gmt::intern(name), parent) { }

ParameterSymbol::ParameterSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: SyntaxSymbol(name, SyntaxKind::Parameter, parent) { }

StructSymbol* ParameterSymbol::get_type() const
{
	return m_type;
}

void ParameterSymbol::set_type(StructSymbol* type)
{
	m_type = type;
}