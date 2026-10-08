#pragma once
#include <shard/semantic/SymbolTable.hpp>

namespace shard
{
	template<typename T, typename... Args>
	gmt::Ref<T> SymbolTable::bind_symbol(gmt::Ref<SyntaxNode> node, Args&&... args)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "bound symbol must derive from SyntaxSymbol");

		gmt::Ref<T> symbol = m_arena.emplace<T>(std::forward<Args>(args)...);
		m_nodeToSymbolMap[node] = symbol;
		m_symbolToNodeMap[symbol] = node;
		return symbol;
	}

	template<typename T>
	gmt::Ref<T> SymbolTable::bind_symbol(gmt::Ref<SyntaxNode> node, gmt::Ref<T> symbol)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "bound symbol must derive from SyntaxSymbol");

		m_nodeToSymbolMap[node] = symbol;
		m_symbolToNodeMap[symbol] = node;
		return symbol;
	}

	template<typename T, typename... Args>
	gmt::Ref<T> SymbolTable::emplace(Args&&... args)
	{
		return m_arena.emplace<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	gmt::Span<T> SymbolTable::emplace_array(std::size_t count)
	{
		return m_arena.emplace_array<T>(count);
	}
}
