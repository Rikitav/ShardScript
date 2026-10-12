#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <unordered_map>
#include <optional>
#include <vector>

namespace shard
{
	class SHARD_API SymbolTable
	{
		gmt::Arena m_arena;
		gmt::Ref<NamespaceSymbol> m_rootNamespace;

		gmt::Ref<StructSymbol> m_void;
		gmt::Ref<StructSymbol> m_boolean;
		gmt::Ref<StructSymbol> m_integer;
		gmt::Ref<StructSymbol> m_double;
		gmt::Ref<StructSymbol> m_char;
		gmt::Ref<StructSymbol> m_string;
		gmt::Ref<StructSymbol> m_array;
		gmt::Ref<StructSymbol> m_nativeInteger;
		gmt::Ref<StructSymbol> m_byte;

		std::unordered_map<gmt::Ref<const SyntaxNode>, gmt::Ref<SyntaxSymbol>> m_nodeToSymbolMap;
		std::unordered_map<gmt::Ref<SyntaxSymbol>, gmt::Ref<const SyntaxNode>> m_symbolToNodeMap;

		// hash -> candidates; collisions resolved by full structural comparison
		std::unordered_map<std::size_t, std::vector<gmt::Ref<ArrayTypeSymbol>>> m_arrayTypes;
		std::unordered_map<std::size_t, std::vector<gmt::Ref<GenericTypeSymbol>>> m_genericTypes;

	public:
		SymbolTable();
		~SymbolTable() = default;

		SymbolTable(const SymbolTable&) = delete;
		SymbolTable& operator=(const SymbolTable&) = delete;

		gmt::Arena& get_arena();
		gmt::Ref<NamespaceSymbol> get_root_namespace() const;

		template<typename T, typename... Args>
		gmt::Ref<T> emplace(Args&&... args);

		template<typename T>
		gmt::Span<T> emplace_array(std::size_t count);

		template<typename T, typename... Args>
		gmt::Ref<T> bind_symbol(gmt::Ref<const SyntaxNode> node, Args&&... args);

		template<typename T>
		gmt::Ref<T> bind_symbol(gmt::Ref<const SyntaxNode> node, gmt::Ref<T> symbol);

		gmt::Ref<SyntaxSymbol> lookup_symbol(gmt::Ref<const SyntaxNode> node);
		gmt::Ref<const SyntaxNode> lookup_node(gmt::Ref<SyntaxSymbol> symbol);

		gmt::Ref<ArrayTypeSymbol> find_array_type(gmt::Ref<StructSymbol> underlaying) const;
		gmt::Ref<ArrayTypeSymbol> find_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size) const;
		gmt::Ref<GenericTypeSymbol> find_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments) const;

		gmt::Ref<ArrayTypeSymbol> find_or_create_array_type(gmt::Ref<StructSymbol> underlaying);
		gmt::Ref<ArrayTypeSymbol> find_or_create_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size);
		gmt::Ref<GenericTypeSymbol> find_or_create_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments);

		gmt::Ref<StructSymbol> get_void_primitive();
		gmt::Ref<StructSymbol> get_boolean_primitive();
		gmt::Ref<StructSymbol> get_integer_primitive();
		gmt::Ref<StructSymbol> get_double_primitive();
		gmt::Ref<StructSymbol> get_char_primitive();
		gmt::Ref<StructSymbol> get_string_primitive();
		gmt::Ref<StructSymbol> get_nint_primitive();
		gmt::Ref<StructSymbol> get_byte_primitive();
	};
}

#include <shard/semantic/SymbolTable.impl.hpp>
