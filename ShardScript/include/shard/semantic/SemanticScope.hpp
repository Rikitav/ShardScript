#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <gmt/Interner.hpp>
#include <gmt/Ref.hpp>

#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace shard
{
	class SHARD_API SemanticScope
	{
		std::unordered_map<std::wstring_view, gmt::Ref<SyntaxSymbol>> m_symbols;

		SemanticScope* m_parent;
		gmt::Ref<SyntaxSymbol> m_owner;
		gmt::Ref<NamespaceSymbol> m_namespace;

		// TODO: move to expression binder
		std::unordered_set<gmt::Ref<NamespaceSymbol>> m_importedNamespaces;

	public:
		SemanticScope(gmt::Ref<SyntaxSymbol> owner, SemanticScope* parent);

		SemanticScope(const SemanticScope& other) = delete;
		SemanticScope& operator=(const SemanticScope& other) = delete;

		SemanticScope* get_parent() const;
		gmt::Ref<SyntaxSymbol> get_owner() const;

		gmt::Ref<NamespaceSymbol> get_namespace() const;
		void set_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol);

		// TODO: move to expression binder
		const std::unordered_set<gmt::Ref<NamespaceSymbol>>& get_imported_namespaces() const;

		// TODO: move to expression binder
		void add_imported_namespace(gmt::Ref<NamespaceSymbol> namespaceSymbol);

		gmt::Ref<SyntaxSymbol> find_local(gmt::StringReference name) const;
		gmt::Ref<SyntaxSymbol> find_local(std::wstring_view name) const;
		gmt::Ref<SyntaxSymbol> lookup(gmt::StringReference name) const;
		gmt::Ref<SyntaxSymbol> lookup(std::wstring_view name) const;

		void declare_symbol(gmt::Ref<SyntaxSymbol> symbol);
	};
}
