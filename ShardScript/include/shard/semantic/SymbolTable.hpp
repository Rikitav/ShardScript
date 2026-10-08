#pragma once
#include <shard/Definitions.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Ref.hpp>

#include <unordered_map>
#include <optional>

namespace shard
{
	class TypeSymbol;

	class SHARD_API SymbolTable
	{
		gmt::Arena m_arena;
		gmt::Ref<NamespaceSymbol> m_root_namespace;

		std::unordered_map<gmt::Ref<SyntaxNode>, gmt::Ref<SyntaxSymbol>> nodeToSymbolMap;
		std::unordered_map<gmt::Ref<SyntaxSymbol>, gmt::Ref<SyntaxNode>> symbolToNodeMap;

	public:
		struct Primitives
		{
			static SHARD_API TypeSymbol* Void;
			static SHARD_API TypeSymbol* Null;
			static SHARD_API TypeSymbol* Any;

			static SHARD_API TypeSymbol* Boolean;
			static SHARD_API TypeSymbol* Integer;
			static SHARD_API TypeSymbol* Double;
			static SHARD_API TypeSymbol* Char;
			static SHARD_API TypeSymbol* String;
			static SHARD_API TypeSymbol* Array;
			static SHARD_API TypeSymbol* NativeInteger;
			static SHARD_API TypeSymbol* Byte;
		};

		SymbolTable();
		~SymbolTable() = default;

		SymbolTable(const SymbolTable&) = delete;
		SymbolTable& operator=(const SymbolTable&) = delete;

		gmt::Arena& get_arena();
		gmt::Ref<NamespaceSymbol> get_root_namespace() const;

		template<typename T, typename... Args>
		gmt::Ref<T> emplace(Args&&... args);

		template<typename T>
		gmt::Span<T> allocate_array(std::size_t count);

		template<typename T, typename... Args>
		gmt::Ref<T> bind_symbol(gmt::Ref<SyntaxNode> node, Args&&... args);

		template<typename T>
		gmt::Ref<T> bind_symbol(gmt::Ref<SyntaxNode> node, gmt::Ref<T> symbol);

		std::optional<gmt::Ref<SyntaxSymbol>> lookup_symbol(gmt::Ref<SyntaxNode> node);
		std::optional<gmt::Ref<SyntaxNode>> lookup_node(gmt::Ref<SyntaxSymbol> symbol);
	};

	extern SHARD_API TypeSymbol*& TYPE_VOID;
	extern SHARD_API TypeSymbol*& TYPE_NULL;
	extern SHARD_API TypeSymbol*& TYPE_ANY;
	extern SHARD_API TypeSymbol*& TYPE_NINT;

	extern SHARD_API TypeSymbol*& TYPE_BOOL;
	extern SHARD_API TypeSymbol*& TYPE_INT;
	extern SHARD_API TypeSymbol*& TYPE_DOUBLE;
	extern SHARD_API TypeSymbol*& TYPE_CHAR;
	extern SHARD_API TypeSymbol*& TYPE_STRING;
	extern SHARD_API TypeSymbol*& TYPE_ARRAY;
	extern SHARD_API TypeSymbol*& TYPE_BYTE;
}

#include <shard/semantic/SymbolTable.impl.hpp>
