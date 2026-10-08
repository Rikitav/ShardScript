#include <shard/semantic/SymbolTable.hpp>

using namespace shard;

gmt::Ref<StructSymbol> SymbolTable::Primitives::Void;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Null;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Any;

gmt::Ref<StructSymbol> SymbolTable::Primitives::Boolean;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Integer;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Double;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Char;
gmt::Ref<StructSymbol> SymbolTable::Primitives::String;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Array;
gmt::Ref<StructSymbol> SymbolTable::Primitives::NativeInteger;
gmt::Ref<StructSymbol> SymbolTable::Primitives::Byte;

gmt::Ref<StructSymbol>& shard::TYPE_VOID = shard::SymbolTable::Primitives::Void;
gmt::Ref<StructSymbol>& shard::TYPE_NULL = shard::SymbolTable::Primitives::Null;
gmt::Ref<StructSymbol>& shard::TYPE_ANY = shard::SymbolTable::Primitives::Any;
gmt::Ref<StructSymbol>& shard::TYPE_NINT = shard::SymbolTable::Primitives::NativeInteger;

gmt::Ref<StructSymbol>& shard::TYPE_BOOL = shard::SymbolTable::Primitives::Boolean;
gmt::Ref<StructSymbol>& shard::TYPE_INT = shard::SymbolTable::Primitives::Integer;
gmt::Ref<StructSymbol>& shard::TYPE_DOUBLE = shard::SymbolTable::Primitives::Double;
gmt::Ref<StructSymbol>& shard::TYPE_CHAR = shard::SymbolTable::Primitives::Char;
gmt::Ref<StructSymbol>& shard::TYPE_STRING = shard::SymbolTable::Primitives::String;
gmt::Ref<StructSymbol>& shard::TYPE_ARRAY = shard::SymbolTable::Primitives::Array;
gmt::Ref<StructSymbol>& shard::TYPE_BYTE = shard::SymbolTable::Primitives::Byte;

SymbolTable::SymbolTable()
	: m_root_namespace(m_arena.emplace<NamespaceSymbol>(L"", gmt::nullref))
{ }

gmt::Arena& SymbolTable::get_arena()
{
	return m_arena;
}

gmt::Ref<NamespaceSymbol> SymbolTable::get_root_namespace() const
{
	return m_root_namespace;
}

std::optional<gmt::Ref<SyntaxSymbol>> SymbolTable::lookup_symbol(gmt::Ref<SyntaxNode> node)
{
	auto choice = nodeToSymbolMap.find(node);
	return choice == nodeToSymbolMap.end() ? std::nullopt : std::optional<gmt::Ref<SyntaxSymbol>>(choice->second);
}

std::optional<gmt::Ref<SyntaxNode>> SymbolTable::lookup_node(gmt::Ref<SyntaxSymbol> symbol)
{
	auto choice = symbolToNodeMap.find(symbol);
	return choice == symbolToNodeMap.end() ? std::nullopt : std::optional<gmt::Ref<SyntaxNode>>(choice->second);
}
