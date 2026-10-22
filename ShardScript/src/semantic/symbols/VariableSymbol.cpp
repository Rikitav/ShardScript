#include <shard/semantic/symbols/VariableSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

VariableSymbol::VariableSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: VariableSymbol(gmt::intern(name), parent) { }

VariableSymbol::VariableSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: SyntaxSymbol(name, SyntaxKind::VariableStatement, parent) { }

gmt::Ref<StructSymbol> VariableSymbol::get_type() const
{
	return m_type;
}

void VariableSymbol::set_type(gmt::Ref<StructSymbol> type)
{
	m_type = type;
}