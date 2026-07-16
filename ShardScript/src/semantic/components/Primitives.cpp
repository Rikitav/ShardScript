#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>

using namespace shard;

namespace
{
	template<typename T>
	static inline void make_primitive(TypeSymbol*& symbol, const wchar_t* name, const size_t memoryBytesSize)
	{
		symbol = new T(name);
		symbol->MemoryBytesSize = memoryBytesSize;
		symbol->LayoutingState = TypeLayoutingState::Visited;
		symbol->AnalysisState = SymbolAnalysisState::Ready;
	}
}

void SymbolTable::ResolvePrimitives(SymbolTable* globalTable)
{
	// ============================================================================
	// Matters
	// ============================================================================
	make_primitive<StructSymbol>(SymbolTable::Primitives::Void, L"Void", 0);
	make_primitive<StructSymbol>(SymbolTable::Primitives::Any, L"Any", 0);
	make_primitive<StructSymbol>(SymbolTable::Primitives::Null, L"Null", 0);
	make_primitive<StructSymbol>(SymbolTable::Primitives::NativeInteger, L"IntPtr", sizeof(void*));

	// ============================================================================
	// Primitives
	// ============================================================================
	make_primitive<StructSymbol>(SymbolTable::Primitives::Boolean, L"Boolean", sizeof(bool));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Integer, L"Integer", sizeof(std::int64_t));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Double, L"Double", sizeof(double));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Char, L"Char", sizeof(wchar_t));
	make_primitive<StructSymbol>(SymbolTable::Primitives::Byte, L"Byte", sizeof(std::uint8_t));
	make_primitive<ClassSymbol>(SymbolTable::Primitives::String, L"String", sizeof(std::int64_t) + sizeof(wchar_t*));	// long _length + char[] _data
	make_primitive<ClassSymbol>(SymbolTable::Primitives::Array, L"Array", sizeof(std::int64_t));						// long _length
}
