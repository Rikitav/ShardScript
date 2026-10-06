#pragma once
#include <shard/semantic/SymbolTable.hpp>

namespace shard
{
	template<typename T, typename... Args>
	gmt::Ref<T> SymbolTable::bind_symbol(gmt::Ref<SyntaxNode> node, Args&&... args)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "bound symbol must derive from SyntaxSymbol");

		gmt::Ref<T> symbol = m_arena.emplace<T>(std::forward<Args>(args)...);
		nodeToSymbolMap[node] = symbol;
		symbolToNodeMap[symbol] = node;
		return symbol;
	}

	template<typename T, typename... Args>
	gmt::Ref<T> SymbolTable::implicit_symbol(Args&&... args)
	{
		static_assert(std::is_base_of_v<SyntaxSymbol, T>, "implicit symbol must derive from SyntaxSymbol");

		return m_arena.emplace<T>(std::forward<Args>(args)...);
	}
}
