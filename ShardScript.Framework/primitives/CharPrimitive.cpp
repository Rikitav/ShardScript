#include <string>
#include <cctype>
#include <new>
#include <vector>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// Char methods
static ObjectInstance* ToString(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	std::wstring str(1, value);
	return context.Collector.FromValue(str);
}

static ObjectInstance* ToUpper(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	wchar_t result = static_cast<wchar_t>(towupper(value));
	return context.Collector.FromValue(result);
}

static ObjectInstance* ToLower(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	wchar_t result = static_cast<wchar_t>(towlower(value));
	return context.Collector.FromValue(result);
}

static ObjectInstance* IsDigit(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswdigit(value) != 0;
	return context.Collector.FromValue(result);
}

static ObjectInstance* IsLetter(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswalpha(value) != 0;
	return context.Collector.FromValue(result);
}

static ObjectInstance* IsWhiteSpace(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	wchar_t value = instance->AsCharacter();
	bool result = iswspace(value) != 0;
	return context.Collector.FromValue(result);
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
