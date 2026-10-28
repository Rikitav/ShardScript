#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <optional>

using namespace shard;

NamespaceSymbol::NamespaceSymbol(std::wstring_view name, NamespaceSymbol* parent)
	: SyntaxSymbol(name, SyntaxKind::NamespaceDirective, parent) { }

NamespaceSymbol::NamespaceSymbol(gmt::StringReference name, NamespaceSymbol* parent)
	: SyntaxSymbol(name, SyntaxKind::NamespaceDirective, parent) { }

void NamespaceSymbol::on_symbol_declared(SyntaxSymbol* symbol)
{
	if (symbol->is_type() || symbol->is_member())
		members.push_back(symbol);
}

const gmt::FlatMap<gmt::StringReference, NamespaceSymbol*>& NamespaceSymbol::get_children() const
{
	return m_children;
}

NamespaceSymbol* NamespaceSymbol::lookup(gmt::StringReference name) const
{
	NamespaceSymbol* const* child = m_children.find(name);
	if (child == nullptr)
		return nullptr;

	return *child;
}

NamespaceSymbol* NamespaceSymbol::lookup(std::wstring_view name) const
{
	gmt::StringReference key = gmt::intern(name);
	return lookup(key);
}

NamespaceSymbol* NamespaceSymbol::lookup_or_create(gmt::StringReference name, SymbolTable& table)
{
	if (NamespaceSymbol* const* existing = m_children.find(name))
		return *existing;

	std::optional<NamespaceSymbol*> self = table.get_arena().ref_from_ptr(this);
	if (!self.has_value())
		return nullptr;

	NamespaceSymbol* child = table.emplace<NamespaceSymbol>(name, *self);
	m_children.try_emplace(name, child);
	return child;
}

NamespaceSymbol* NamespaceSymbol::lookup_or_create(std::wstring_view name, SymbolTable& table)
{
	gmt::StringReference key = gmt::intern(name);
	return lookup_or_create(key, table);
}