#include <shard/semantic/SemanticScope.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <gmt/Containers.hpp>

#include <stdexcept>

using namespace shard;

namespace
{
	bool is_overloadable_kind(const SyntaxKind kind)
	{
		switch (kind)
		{
			case SyntaxKind::FunctionDeclaration:
			case SyntaxKind::ConstructorDeclaration:
			case SyntaxKind::AccessorDeclaration:
			case SyntaxKind::OperatorDeclaration:
				return true;

			default:
				return false;
		}
	}
}

SemanticScope::SemanticScope(gmt::Ref<SyntaxSymbol> owner, SemanticScope* parent)
	: m_parent(parent), m_owner(owner)
{
	if (parent != nullptr)
		m_namespace = parent->m_namespace;
}

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

const gmt::unordered_set<gmt::Ref<NamespaceSymbol>>& SemanticScope::get_imported_namespaces() const
{
	return m_importedNamespaces;
}

void SemanticScope::add_imported_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol)
{
	m_importedNamespaces.insert(namespaceSymbol);
}

gmt::Ref<SyntaxSymbol> SemanticScope::find_local(gmt::StringReference name) const
{
	return find_local(gmt::resolve(name));
}

gmt::Ref<SyntaxSymbol> SemanticScope::find_local(std::wstring_view name) const
{
	const auto lookup = m_symbols.find(name);
	if (lookup == m_symbols.end())
		return gmt::nullref;

	return lookup->second;
}

gmt::Ref<SyntaxSymbol> SemanticScope::lookup(gmt::StringReference name) const
{
	return lookup(gmt::resolve(name));
}

gmt::Ref<SyntaxSymbol> SemanticScope::lookup(std::wstring_view name) const
{
	gmt::Ref<SyntaxSymbol> local = find_local(name);
	if (!local.is_null())
		return local;

	if (m_parent != nullptr)
		return m_parent->lookup(name);

	return gmt::nullref;
}

std::span<const gmt::Ref<SyntaxSymbol>> SemanticScope::find_overloads(std::wstring_view name) const
{
	for (const SemanticScope* scope = this; scope != nullptr; scope = scope->m_parent)
	{
		const auto symbol = scope->m_symbols.find(name);
		if (symbol == scope->m_symbols.end())
			continue;

		const auto overloads = scope->m_overloads.find(name);
		if (overloads == scope->m_overloads.end())
			return {};

		return overloads->second;
	}

	return {};
}

void SemanticScope::declare_symbol(gmt::Ref<SyntaxSymbol> symbol)
{
	if (symbol.is_null())
		throw std::runtime_error("tried to declare null symbol");

	const std::wstring_view name = symbol->get_name();
	const auto find = m_symbols.find(name);

	if (find == m_symbols.end())
	{
		m_symbols.emplace(name, symbol);
		return;
	}

	if (find->second == symbol)
		return;

	// functions, constructors, operators and accessors may overload each
	// other within the same scope; the name keeps resolving to the first
	// declared symbol and the rest are reachable through find_overloads
	bool bothOverloadable = is_overloadable_kind(find->second->get_kind()) && is_overloadable_kind(symbol->get_kind());
	if (bothOverloadable)
	{
		m_overloads[name].push_back(symbol);
		return;
	}

	throw std::runtime_error(("symbol already defined: " + gmt::string(name.begin(), name.end())).c_str());
}
