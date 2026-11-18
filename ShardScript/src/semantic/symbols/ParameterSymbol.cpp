#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

ParameterSymbol::ParameterSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: ParameterSymbol(gmt::intern(name), parent) { }

ParameterSymbol::ParameterSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: SyntaxSymbol(name, SyntaxKind::Parameter, parent) { }

ParameterSymbol::ParameterSymbol(gmt::StringReference name, SyntaxSymbol* parent, EscapeContract escapeContract)
	: SyntaxSymbol(name, SyntaxKind::Parameter, parent), m_escapeContract(escapeContract) { }

StructSymbol* ParameterSymbol::get_type() const
{
	return m_type;
}

void ParameterSymbol::set_type(StructSymbol* type)
{
	m_type = type;
}

EscapeContract ParameterSymbol::get_escape_contract() const
{
	return m_escapeContract;
}

void ParameterSymbol::set_escape_contract(EscapeContract escapeContract)
{
	m_escapeContract = escapeContract;
}