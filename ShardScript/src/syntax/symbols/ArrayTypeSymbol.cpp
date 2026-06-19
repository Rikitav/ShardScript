#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <vector>
#include <string>

using namespace shard;

ConstructorSymbol* ArrayTypeSymbol::FindConstructor(const std::vector<TypeSymbol*>& parameterTypes)
{
	return SymbolTable::Primitives::Array->FindConstructor(parameterTypes);
}

MethodSymbol* ArrayTypeSymbol::FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes)
{
	return SymbolTable::Primitives::Array->FindMethod(name, parameterTypes);
}

MethodSymbol* ArrayTypeSymbol::FindInterfaceImplementation(MethodSymbol* interfaceMethod)
{
	return SymbolTable::Primitives::Array->FindInterfaceImplementation(interfaceMethod);
}

IndexatorSymbol* ArrayTypeSymbol::FindIndexator(const std::vector<TypeSymbol*>& parameterTypes)
{
	return SymbolTable::Primitives::Array->FindIndexator(parameterTypes);
}

FieldSymbol* ArrayTypeSymbol::FindField(std::wstring& name)
{
	return SymbolTable::Primitives::Array->FindField(name);
}

PropertySymbol* ArrayTypeSymbol::FindProperty(std::wstring& name)
{
	return SymbolTable::Primitives::Array->FindProperty(name);
}
