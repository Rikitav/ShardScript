#include <shard/semantic/symbols/IndexatorSymbol.hpp>

#include <gmt/Interner.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

using namespace shard;

IndexatorSymbol::IndexatorSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: IndexatorSymbol(gmt::intern(name), parent) { }

IndexatorSymbol::IndexatorSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: PropertySymbol(name, SyntaxKind::IndexatorDeclaration, parent) { }

const std::vector<gmt::Ref<ParameterSymbol>>& IndexatorSymbol::get_parameters() const
{
	return m_parameters;
}

void IndexatorSymbol::on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol)
{
	if (symbol->get_kind() == SyntaxKind::Parameter)
		m_parameters.push_back(symbol.as<ParameterSymbol>());

	gmt::Ref<AccessorSymbol> getter = get_getter();
	if (!getter.is_null())
		getter->on_symbol_declared(symbol);

	gmt::Ref<AccessorSymbol> setter = get_setter();
	if (!setter.is_null())
		setter->on_symbol_declared(symbol);
}
