#include <shard/semantic/SymbolTable.hpp>

using namespace shard;

SymbolTable::SymbolTable()
	: m_rootNamespace(m_arena.emplace<NamespaceSymbol>(L"", gmt::nullref))
{ }

gmt::Arena& SymbolTable::get_arena()
{
	return m_arena;
}

gmt::Ref<NamespaceSymbol> SymbolTable::get_root_namespace() const
{
	return m_rootNamespace;
}

gmt::Ref<SyntaxSymbol> SymbolTable::lookup_symbol(gmt::Ref<const SyntaxNode> node)
{
	auto choice = m_nodeToSymbolMap.find(node);
	if (choice == m_nodeToSymbolMap.end())
		return gmt::nullref;

	return choice->second;
}

gmt::Ref<const SyntaxNode> SymbolTable::lookup_node(gmt::Ref<SyntaxSymbol> symbol)
{
	auto choice = m_symbolToNodeMap.find(symbol);
	if (choice == m_symbolToNodeMap.end())
		return gmt::nullref;

	return choice->second;
}

gmt::Ref<StructSymbol> SymbolTable::get_void_primitive()
{
	return m_void;
}

gmt::Ref<StructSymbol> SymbolTable::get_boolean_primitive()
{
	return m_boolean;
}

gmt::Ref<StructSymbol> SymbolTable::get_integer_primitive()
{
	return m_integer;
}

gmt::Ref<StructSymbol> SymbolTable::get_double_primitive()
{
	return m_double;
}

gmt::Ref<StructSymbol> SymbolTable::get_char_primitive()
{
	return m_char;
}

gmt::Ref<StructSymbol> SymbolTable::get_string_primitive()
{
	return m_string;
}

gmt::Ref<StructSymbol> SymbolTable::get_nint_primitive()
{
	return m_nativeInteger;
}

gmt::Ref<StructSymbol> SymbolTable::get_byte_primitive()
{
	return m_byte;
}
