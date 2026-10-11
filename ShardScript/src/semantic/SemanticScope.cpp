#include <shard/semantic/SemanticScope.hpp>

#include <stdexcept>

using namespace shard;

SemanticScope::SemanticScope(gmt::Ref<SyntaxSymbol> owner, SemanticScope* parent)
	: m_parent(parent), m_owner(owner) { }

SemanticScope* SemanticScope::get_parent() const
{
	return m_parent;
}

gmt::Ref<SyntaxSymbol> SemanticScope::get_owner() const
{
	return m_owner;
}

gmt::Ref<NamespaceSymbol> SemanticScope::get_namespace() const
{
	return m_namespace;
}

void SemanticScope::set_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol)
{
	m_namespace = namespaceSymbol;
}

const std::unordered_set<gmt::Ref<NamespaceSymbol>>& SemanticScope::get_imported_namespaces() const
{
	return m_importedNamespaces;
}

void SemanticScope::add_imported_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol)
{
	m_importedNamespaces.insert(namespaceSymbol);
}

gmt::Ref<SyntaxSymbol> SemanticScope::find_local(gmt::StringReference name) const
{
	const auto lookup = m_symbols.find(name);
	if (lookup == m_symbols.end())
		return gmt::nullref;

	return lookup->second;
}

gmt::Ref<SyntaxSymbol> SemanticScope::lookup(gmt::StringReference name) const
{
	gmt::Ref<SyntaxSymbol> local = find_local(name);
	if (!local.is_null())
		return local;

	if (!m_namespace.is_null())
	{
		for (const gmt::Ref<SyntaxSymbol>& member : m_namespace->members)
		{
			if (!member.is_null() && member->get_name_reference() == name)
				return member;
		}
	}

	if (m_parent != nullptr)
		return m_parent->lookup(name);

	return gmt::nullref;
}

gmt::Ref<SyntaxSymbol> SemanticScope::lookup(std::wstring_view name) const
{
	return lookup(gmt::intern(name));
}

void SemanticScope::declare_symbol(gmt::Ref<SyntaxSymbol> symbol)
{
	if (symbol.is_null())
		throw std::runtime_error("tried to declare null symbol");

	gmt::StringReference name = symbol->get_name_reference();
	const auto find = m_symbols.find(name);

	if (find != m_symbols.end() && find->second != symbol)
		throw std::runtime_error("symbol already defined: " + std::string(symbol->get_name().begin(), symbol->get_name().end()));

	m_symbols.emplace(name, symbol);
}
