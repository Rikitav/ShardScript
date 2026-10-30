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
		NamespaceSymbol* m_rootNamespace = nullptr;

		StructSymbol* m_void = nullptr;
		StructSymbol* m_boolean = nullptr;
		StructSymbol* m_integer = nullptr;
		StructSymbol* m_double = nullptr;
		StructSymbol* m_char = nullptr;
		StructSymbol* m_string = nullptr;
		StructSymbol* m_array = nullptr;
		StructSymbol* m_nativeInteger = nullptr;
		StructSymbol* m_byte = nullptr;

		gmt::unordered_map<const SyntaxNode*, SyntaxSymbol*> m_nodeToSymbolMap;
		gmt::unordered_map<SyntaxSymbol*, const SyntaxNode*> m_symbolToNodeMap;

		// inferred or explicit call-site type arguments per invocation node.
		// consumed by the emitter to build instantiations
		gmt::unordered_map<const SyntaxNode*, gmt::vector<StructSymbol*>> m_nodeTypeArguments;

		// composite symbols registry
		SymbolInterner m_interner;

		// well-known standard library slots
		std::array<SyntaxSymbol*, static_cast<std::size_t>(StandardTypes::Count)> m_standardTypes{};

		// standard components resolvers
		void resolve_primitives();
		void resolve_interfaces();
		void resolve_primitive_printables();
		void resolve_enumerables();
		void resolve_exceptions();
		void resolve_async_types();

	public:
		SymbolTable();
		~SymbolTable() = default;

		SymbolTable(const SymbolTable&) = delete;
		SymbolTable& operator=(const SymbolTable&) = delete;

		gmt::Arena& get_arena();
		NamespaceSymbol* get_root_namespace() const;

		template<typename T, typename... Args>
		T* emplace(Args&&... args);

		template<typename T>
		std::span<T> emplace_array(std::size_t count);

		template<typename T, typename... Args> requires std::is_constructible_v<T, Args&&...>
		T* bind_symbol(const SyntaxNode* node, Args&&... args);

		template<typename T>
		T* bind_symbol(const SyntaxNode* node, T* symbol);

		SyntaxSymbol* lookup_symbol(const SyntaxNode* node);
		const SyntaxNode* lookup_node(SyntaxSymbol* symbol);

		void bind_type_arguments(const SyntaxNode* node, std::span<StructSymbol* const> typeArguments);
		std::span<StructSymbol* const> lookup_type_arguments(const SyntaxNode* node) const;

		ArrayTypeSymbol* find_array_type(StructSymbol* underlaying) const;
		ArrayTypeSymbol* find_array_type(StructSymbol* underlaying, std::size_t size) const;
		GenericTypeSymbol* find_generic_type(StructSymbol* underlaying, std::span<StructSymbol* const> typeArguments) const;
		GenericMethodSymbol* find_generic_method(MethodSymbol* underlaying, std::span<StructSymbol* const> typeArguments) const;
		NullableTypeSymbol* find_nullable_type(StructSymbol* underlaying) const;

		ArrayTypeSymbol* find_or_create_array_type(StructSymbol* underlaying);
		ArrayTypeSymbol* find_or_create_array_type(StructSymbol* underlaying, std::size_t size);
		GenericTypeSymbol* find_or_create_generic_type(StructSymbol* underlaying, std::span<StructSymbol* const> typeArguments);
		GenericMethodSymbol* find_or_create_generic_method(MethodSymbol* underlaying, std::span<StructSymbol* const> typeArguments, StructSymbol* returnType);
		NullableTypeSymbol* find_or_create_nullable_type(StructSymbol* underlaying);

		StructSymbol* get_void_primitive() const;
		StructSymbol* get_boolean_primitive() const;
		StructSymbol* get_integer_primitive() const;
		StructSymbol* get_double_primitive() const;
		StructSymbol* get_char_primitive() const;
		StructSymbol* get_string_primitive() const;
		StructSymbol* get_array_primitive() const;
		StructSymbol* get_nint_primitive() const;
		StructSymbol* get_byte_primitive() const;

		SyntaxSymbol* get_standard_type(StandardTypes type);
		void set_standard_type(StandardTypes type, SyntaxSymbol* symbol);
	};
}

#include <shard/semantic/SymbolTable.impl.hpp>
