#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

VariableSymbol::VariableSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: VariableSymbol(gmt::intern(name), parent) { }

VariableSymbol::VariableSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: SyntaxSymbol(name, SyntaxKind::VariableStatement, parent) { }

StructSymbol* VariableSymbol::get_type() const
{
	return m_type;
}

void VariableSymbol::set_type(StructSymbol* type)
{
	m_type = type;
}