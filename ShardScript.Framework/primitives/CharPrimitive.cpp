#include <shard/syntax/SyntaxSymbol.h>
#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/VirtualMachine.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>
#include <cctype>
#include <new>
#include <vector>

#include "PrimitivesLoading.h"

using namespace shard;

// Char methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	std::wstring str(1, value);
	return ObjectInstance::FromValue(str);
}

static ObjectInstance* ToUpper(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	wchar_t result = static_cast<wchar_t>(towupper(value));
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* ToLower(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	wchar_t result = static_cast<wchar_t>(towlower(value));
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* IsDigit(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswdigit(value) != 0;
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* IsLetter(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswalpha(value) != 0;
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* IsWhiteSpace(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswspace(value) != 0;
	return ObjectInstance::FromValue(result);
}

void CharPrimitive::Reflect(TypeSymbol* symbol)
{
	// ToString()
	MethodSymbol* toString = new MethodSymbol(L"ToString", ToString);
	toString->Accesibility = SymbolAccesibility::Public;
	toString->ReturnType = SymbolTable::Primitives::String;
	toString->IsStatic = false;
	symbol->Methods.push_back(toString);
	
	// ToUpper()
	MethodSymbol* toUpper = new MethodSymbol(L"ToUpper", ToUpper);
	toUpper->Accesibility = SymbolAccesibility::Public;
	toUpper->ReturnType = SymbolTable::Primitives::Char;
	toUpper->IsStatic = false;
	symbol->Methods.push_back(toUpper);
	
	// ToLower()
	MethodSymbol* toLower = new MethodSymbol(L"ToLower", ToLower);
	toLower->Accesibility = SymbolAccesibility::Public;
	toLower->ReturnType = SymbolTable::Primitives::Char;
	toLower->IsStatic = false;
	symbol->Methods.push_back(toLower);
	
	// IsDigit()
	MethodSymbol* isDigit = new MethodSymbol(L"IsDigit", IsDigit);
	isDigit->Accesibility = SymbolAccesibility::Public;
	isDigit->ReturnType = SymbolTable::Primitives::Boolean;
	isDigit->IsStatic = false;
	symbol->Methods.push_back(isDigit);
	
	// IsLetter()
	MethodSymbol* isLetter = new MethodSymbol(L"IsLetter", IsLetter);
	isLetter->Accesibility = SymbolAccesibility::Public;
	isLetter->ReturnType = SymbolTable::Primitives::Boolean;
	isLetter->IsStatic = false;
	symbol->Methods.push_back(isLetter);
	
	// IsWhiteSpace()
	MethodSymbol* isWhiteSpace = new MethodSymbol(L"IsWhiteSpace", IsWhiteSpace);
	isWhiteSpace->Accesibility = SymbolAccesibility::Public;
	isWhiteSpace->ReturnType = SymbolTable::Primitives::Boolean;
	isWhiteSpace->IsStatic = false;
	symbol->Methods.push_back(isWhiteSpace);
}
