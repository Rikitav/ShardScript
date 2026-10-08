#pragma once
#include <shard/Definitions.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Interner.hpp>

#include <string_view>
#include <unordered_map>

namespace shard
{
	class SymbolTable;

	class SHARD_API NamespaceSymbol : public SyntaxSymbol
	{
		std::unordered_map<gmt::StringReference, gmt::Ref<NamespaceSymbol>> m_children;

	public:
		NamespaceSymbol(std::wstring_view name, gmt::Ref<NamespaceSymbol> parent);
		virtual ~NamespaceSymbol() = default;

		NamespaceSymbol(const NamespaceSymbol& other) = delete;
		NamespaceSymbol& operator=(const NamespaceSymbol& other) = delete;

		NamespaceSymbol(NamespaceSymbol&& other) = delete;
		NamespaceSymbol& operator=(NamespaceSymbol&& other) = delete;

		gmt::Ref<NamespaceSymbol> lookup(std::wstring_view name) const;
		gmt::Ref<NamespaceSymbol> lookup_or_create(std::wstring_view name, SymbolTable& table);
	};
}
