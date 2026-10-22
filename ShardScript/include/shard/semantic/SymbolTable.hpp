#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/StandardTypes.hpp>
#include <shard/semantic/SymbolInterner.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericMethodSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <array>
#include <span>
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

		gmt::unordered_map<gmt::Ref<const SyntaxNode>, gmt::Ref<SyntaxSymbol>> m_nodeToSymbolMap;
		gmt::unordered_map<gmt::Ref<SyntaxSymbol>, gmt::Ref<const SyntaxNode>> m_symbolToNodeMap;

		// inferred or explicit call-site type arguments per invocation node.
		// consumed by the emitter to build instantiations
		gmt::unordered_map<gmt::Ref<const SyntaxNode>, gmt::vector<gmt::Ref<StructSymbol>>> m_nodeTypeArguments;

		// composite symbols registry
		SymbolInterner m_interner;

		// well-known standard library slots
		std::array<gmt::Ref<SyntaxSymbol>, static_cast<std::size_t>(StandardTypes::Count)> m_standardTypes;

		// standard components resolvers
		void resolve_primitives();
		void resolve_interfaces();
		void resolve_primitive_printables();
		void resolve_enumerables();
		void resolve_exceptions();

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

		template<typename T, typename... Args> requires std::is_constructible_v<T, Args&&...>
		gmt::Ref<T> bind_symbol(gmt::Ref<const SyntaxNode> node, Args&&... args);

		template<typename T>
		gmt::Ref<T> bind_symbol(gmt::Ref<const SyntaxNode> node, gmt::Ref<T> symbol);

		gmt::Ref<SyntaxSymbol> lookup_symbol(gmt::Ref<const SyntaxNode> node);
		gmt::Ref<const SyntaxNode> lookup_node(gmt::Ref<SyntaxSymbol> symbol);

		void bind_type_arguments(gmt::Ref<const SyntaxNode> node, std::span<const gmt::Ref<StructSymbol>> typeArguments);
		std::span<const gmt::Ref<StructSymbol>> lookup_type_arguments(gmt::Ref<const SyntaxNode> node) const;

		gmt::Ref<ArrayTypeSymbol> find_array_type(gmt::Ref<StructSymbol> underlaying) const;
		gmt::Ref<ArrayTypeSymbol> find_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size) const;
		gmt::Ref<GenericTypeSymbol> find_generic_type(gmt::Ref<StructSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments) const;
		gmt::Ref<GenericMethodSymbol> find_generic_method(gmt::Ref<MethodSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments) const;
		gmt::Ref<NullableTypeSymbol> find_nullable_type(gmt::Ref<StructSymbol> underlaying) const;

		gmt::Ref<ArrayTypeSymbol> find_or_create_array_type(gmt::Ref<StructSymbol> underlaying);
		gmt::Ref<ArrayTypeSymbol> find_or_create_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size);
		gmt::Ref<GenericTypeSymbol> find_or_create_generic_type(gmt::Ref<StructSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments);
		gmt::Ref<GenericMethodSymbol> find_or_create_generic_method(gmt::Ref<MethodSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments, gmt::Ref<StructSymbol> returnType);
		gmt::Ref<NullableTypeSymbol> find_or_create_nullable_type(gmt::Ref<StructSymbol> underlaying);

		gmt::Ref<StructSymbol> get_void_primitive() const;
		gmt::Ref<StructSymbol> get_boolean_primitive() const;
		gmt::Ref<StructSymbol> get_integer_primitive() const;
		gmt::Ref<StructSymbol> get_double_primitive() const;
		gmt::Ref<StructSymbol> get_char_primitive() const;
		gmt::Ref<StructSymbol> get_string_primitive() const;
		gmt::Ref<StructSymbol> get_array_primitive() const;
		gmt::Ref<StructSymbol> get_nint_primitive() const;
		gmt::Ref<StructSymbol> get_byte_primitive() const;

		gmt::Ref<SyntaxSymbol> get_standard_type(StandardTypes type);
		void set_standard_type(StandardTypes type, gmt::Ref<SyntaxSymbol> symbol);
	};
}

#include <shard/semantic/SymbolTable.impl.hpp>
