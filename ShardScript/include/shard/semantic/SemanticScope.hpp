#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <gmt/Containers.hpp>
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
		gmt::unordered_map<std::wstring_view, SyntaxSymbol*> m_symbols;
		gmt::unordered_map<std::wstring_view, gmt::vector<SyntaxSymbol*>> m_overloads;

		SemanticScope* m_parent;
		SyntaxSymbol* m_owner = nullptr;
		NamespaceSymbol* m_namespace = nullptr;

		gmt::unordered_set<NamespaceSymbol*> m_importedNamespaces;

	public:
		SemanticScope(SyntaxSymbol* owner, SemanticScope* parent);

		SemanticScope(const SemanticScope& other) = delete;
		SemanticScope& operator=(const SemanticScope& other) = delete;

		SemanticScope* get_parent() const;
		SyntaxSymbol* get_owner() const;

		NamespaceSymbol* get_namespace() const;
		void set_namespace(NamespaceSymbol* namespaceSymbol);

		const gmt::unordered_set<NamespaceSymbol*>& get_imported_namespaces() const;
		void add_imported_namespace(NamespaceSymbol* namespaceSymbol);

		SyntaxSymbol* find_local(gmt::StringReference name) const;
		SyntaxSymbol* find_local(std::wstring_view name) const;
		SyntaxSymbol* lookup(gmt::StringReference name) const;
		SyntaxSymbol* lookup(std::wstring_view name) const;

		// overloads declared in the same scope as the symbol that owns the
		// name; empty when the name resolves to a single symbol
		std::span<SyntaxSymbol* const> find_overloads(std::wstring_view name) const;

		void declare_symbol(SyntaxSymbol* symbol);
	};
}
