#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <gmt/Interner.hpp>
#include <gmt/Ref.hpp>

#include <string_view>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shard
{
	class SHARD_API SemanticScope
	{
		std::unordered_map<std::wstring_view, gmt::Ref<SyntaxSymbol>> m_symbols;
		std::unordered_map<std::wstring_view, std::vector<gmt::Ref<SyntaxSymbol>>> m_overloads;

		SemanticScope* m_parent;
		gmt::Ref<SyntaxSymbol> m_owner;
		gmt::Ref<NamespaceSymbol> m_namespace;

		std::unordered_set<gmt::Ref<NamespaceSymbol>> m_importedNamespaces;

	public:
		SemanticScope(gmt::Ref<SyntaxSymbol> owner, SemanticScope* parent);

		SemanticScope(const SemanticScope& other) = delete;
		SemanticScope& operator=(const SemanticScope& other) = delete;

		SemanticScope* get_parent() const;
		gmt::Ref<SyntaxSymbol> get_owner() const;

		gmt::Ref<NamespaceSymbol> get_namespace() const;
		void set_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol);

		const std::unordered_set<gmt::Ref<NamespaceSymbol>>& get_imported_namespaces() const;
		void add_imported_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol);

		gmt::Ref<SyntaxSymbol> find_local(gmt::StringReference name) const;
		gmt::Ref<SyntaxSymbol> find_local(std::wstring_view name) const;
		gmt::Ref<SyntaxSymbol> lookup(gmt::StringReference name) const;
		gmt::Ref<SyntaxSymbol> lookup(std::wstring_view name) const;

		// overloads declared in the same scope as the symbol that owns the
		// name; empty when the name resolves to a single symbol
		std::span<const gmt::Ref<SyntaxSymbol>> find_overloads(std::wstring_view name) const;

		void declare_symbol(gmt::Ref<SyntaxSymbol> symbol);
	};
}
