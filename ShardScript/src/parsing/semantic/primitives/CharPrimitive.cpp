#include <shard/runtime/interpreter/PrimitiveMathModule.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/semantic/primitives/CharPrimitive.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>
#include <cctype>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

// Char methods
static ObjectInstance* ToString(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	wstring str(1, value);
	return PrimitiveMathModule::CreateInstanceFromValue(str);
}

static ObjectInstance* ToUpper(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	wchar_t result = static_cast<wchar_t>(towupper(value));
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* ToLower(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	wchar_t result = static_cast<wchar_t>(towlower(value));
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* IsDigit(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	bool result = iswdigit(value) != 0;
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* IsLetter(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	bool result = iswalpha(value) != 0;
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* IsWhiteSpace(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wchar_t value = instance->ReadPrimitive<wchar_t>();
	bool result = iswspace(value) != 0;
	return PrimitiveMathModule::CreateInstanceFromValue(result);
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
