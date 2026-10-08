#include <shard/semantic/SymbolTable.hpp>

using namespace shard;

TypeSymbol* SymbolTable::Primitives::Void = nullptr;
TypeSymbol* SymbolTable::Primitives::Null = nullptr;
TypeSymbol* SymbolTable::Primitives::Any = nullptr;

TypeSymbol* SymbolTable::Primitives::Boolean = nullptr;
TypeSymbol* SymbolTable::Primitives::Integer = nullptr;
TypeSymbol* SymbolTable::Primitives::Double = nullptr;
TypeSymbol* SymbolTable::Primitives::Char = nullptr;
TypeSymbol* SymbolTable::Primitives::String = nullptr;
TypeSymbol* SymbolTable::Primitives::Array = nullptr;
TypeSymbol* SymbolTable::Primitives::NativeInteger = nullptr;
TypeSymbol* SymbolTable::Primitives::Byte = nullptr;

TypeSymbol*& shard::TYPE_VOID = shard::SymbolTable::Primitives::Void;
TypeSymbol*& shard::TYPE_NULL = shard::SymbolTable::Primitives::Null;
TypeSymbol*& shard::TYPE_ANY = shard::SymbolTable::Primitives::Any;
TypeSymbol*& shard::TYPE_NINT = shard::SymbolTable::Primitives::NativeInteger;

TypeSymbol*& shard::TYPE_BOOL = shard::SymbolTable::Primitives::Boolean;
TypeSymbol*& shard::TYPE_INT = shard::SymbolTable::Primitives::Integer;
TypeSymbol*& shard::TYPE_DOUBLE = shard::SymbolTable::Primitives::Double;
TypeSymbol*& shard::TYPE_CHAR = shard::SymbolTable::Primitives::Char;
TypeSymbol*& shard::TYPE_STRING = shard::SymbolTable::Primitives::String;
TypeSymbol*& shard::TYPE_ARRAY = shard::SymbolTable::Primitives::Array;
TypeSymbol*& shard::TYPE_BYTE = shard::SymbolTable::Primitives::Byte;

SymbolTable::SymbolTable()
	: m_root_namespace(m_arena.emplace<NamespaceSymbol>(L"", gmt::nullref)) {
}

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
