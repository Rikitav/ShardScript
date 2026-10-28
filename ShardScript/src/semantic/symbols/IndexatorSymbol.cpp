#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Interner.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

using namespace shard;

IndexatorSymbol::IndexatorSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: IndexatorSymbol(gmt::intern(name), parent) { }

IndexatorSymbol::IndexatorSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: PropertySymbol(name, SyntaxKind::IndexatorDeclaration, parent) { }

const gmt::vector<ParameterSymbol*>& IndexatorSymbol::get_parameters() const
{
	return m_parameters;
}

void IndexatorSymbol::on_symbol_declared(SyntaxSymbol* symbol)
{
	if (symbol->get_kind() == SyntaxKind::Parameter)
		m_parameters.push_back(static_cast<ParameterSymbol*>(symbol));

	AccessorSymbol* getter = get_getter();
	if (!(getter == nullptr))
		getter->on_symbol_declared(symbol);

	AccessorSymbol* setter = get_setter();
	if (!(setter == nullptr))
		setter->on_symbol_declared(symbol);
}
