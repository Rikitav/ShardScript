#include <shard/semantic/SymbolTable.hpp>

using namespace shard;

void SymbolTable::resolve_primitives()
{
	m_void = m_arena.emplace<StructSymbol>(L"void", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_boolean = m_arena.emplace<StructSymbol>(L"bool", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_integer = m_arena.emplace<StructSymbol>(L"int", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_double = m_arena.emplace<StructSymbol>(L"double", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_char = m_arena.emplace<StructSymbol>(L"char", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_string = m_arena.emplace<StructSymbol>(L"string", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_array = m_arena.emplace<StructSymbol>(L"Array", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_nativeInteger = m_arena.emplace<StructSymbol>(L"nint", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_byte = m_arena.emplace<StructSymbol>(L"byte", SyntaxKind::StructDeclaration, m_rootNamespace);
}
