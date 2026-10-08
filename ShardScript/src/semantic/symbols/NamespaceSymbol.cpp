#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <optional>

using namespace shard;

NamespaceSymbol::NamespaceSymbol(std::wstring_view name, gmt::Ref<NamespaceSymbol> parent)
	: SyntaxSymbol(name, SyntaxKind::NamespaceDirective, parent) { }

NamespaceSymbol::NamespaceSymbol(gmt::StringReference name, gmt::Ref<NamespaceSymbol> parent)
	: SyntaxSymbol(name, SyntaxKind::NamespaceDirective, parent) { }

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup(gmt::StringReference name) const
{
	const gmt::Ref<NamespaceSymbol>* child = m_children.find(name);
	if (child == nullptr)
		return gmt::nullref;

	return *child;
}

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup(std::wstring_view name) const
{
	gmt::StringReference key = gmt::intern(name);
	return lookup(key);
}

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup_or_create(gmt::StringReference name, SymbolTable& table)
{
	if (const gmt::Ref<NamespaceSymbol>* existing = m_children.find(name))
		return *existing;

	std::optional<gmt::Ref<NamespaceSymbol>> self = table.get_arena().ref_from_ptr(this);
	if (!self.has_value())
		return gmt::nullref;

	gmt::Ref<NamespaceSymbol> child = table.emplace<NamespaceSymbol>(name, *self);
	m_children.try_emplace(name, child);
	return child;
}

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup_or_create(std::wstring_view name, SymbolTable& table)
{
	gmt::StringReference key = gmt::intern(name);
	return lookup_or_create(key, table);
}
