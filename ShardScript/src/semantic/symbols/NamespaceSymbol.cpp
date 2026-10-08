#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <optional>

using namespace shard;

NamespaceSymbol::NamespaceSymbol(std::wstring_view name, gmt::Ref<NamespaceSymbol> parent)
	: SyntaxSymbol(name, SyntaxKind::NamespaceDirective, parent) { }

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup(std::wstring_view name) const
{
	auto it = m_children.find(gmt::intern(name));
	if (it == m_children.end())
		return gmt::nullref;

	return it->second;
}

gmt::Ref<NamespaceSymbol> NamespaceSymbol::lookup_or_create(std::wstring_view name, SymbolTable& table)
{
	gmt::StringReference key = gmt::intern(name);

	auto it = m_children.find(key);
	if (it != m_children.end())
		return it->second;

	std::optional<gmt::Ref<NamespaceSymbol>> self = table.get_arena().ref_from_ptr(this);
	if (!self.has_value())
		return gmt::nullref;

	gmt::Ref<NamespaceSymbol> child = table.emplace<NamespaceSymbol>(name, *self);

	m_children.emplace(key, child);
	return child;
}
