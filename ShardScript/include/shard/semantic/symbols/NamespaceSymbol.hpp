#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/FlatMap.hpp>
#include <gmt/Interner.hpp>

#include <string_view>
#include <vector>

namespace shard
{
	class SymbolTable;

	class SHARD_API NamespaceSymbol : public SyntaxSymbol
	{
		gmt::FlatMap<gmt::StringReference, NamespaceSymbol*> m_children;

	public:
		gmt::vector<SyntaxSymbol*> members;

		NamespaceSymbol(std::wstring_view name, NamespaceSymbol* parent);
		NamespaceSymbol(gmt::StringReference name, NamespaceSymbol* parent);
		virtual ~NamespaceSymbol() = default;

		NamespaceSymbol(const NamespaceSymbol& other) = delete;
		NamespaceSymbol& operator=(const NamespaceSymbol& other) = delete;

		NamespaceSymbol(NamespaceSymbol&& other) = delete;
		NamespaceSymbol& operator=(NamespaceSymbol&& other) = delete;

		void on_symbol_declared(SyntaxSymbol* symbol) override;

		const gmt::FlatMap<gmt::StringReference, NamespaceSymbol*>& get_children() const;

		NamespaceSymbol* lookup(std::wstring_view name) const;
		NamespaceSymbol* lookup(gmt::StringReference name) const;
		NamespaceSymbol* lookup_or_create(std::wstring_view name, SymbolTable& table);
		NamespaceSymbol* lookup_or_create(gmt::StringReference, SymbolTable& table);
	};
}
