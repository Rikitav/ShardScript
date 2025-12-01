#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/framework/primitives/StringPrimitive.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

// String methods
static ObjectInstance* IsEmpty(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	return ObjectInstance::FromValue(value.size() == 0);
}

static ObjectInstance* GetLength(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	return ObjectInstance::FromValue(static_cast<int>(value.size()));
}

static ObjectInstance* Substring(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* startArg = arguments->TryFind(L"start");
	if (startArg == nullptr)
		return ObjectInstance::FromValue(L"");
	
	int start = startArg->ReadPrimitive<int>();
	if (start < 0)
		start = 0;

	if (start >= static_cast<int>(value.size()))
		return ObjectInstance::FromValue(L"");
	
	ObjectInstance* lengthArg = arguments->TryFind(L"length");
	if (lengthArg != nullptr)
	{
		int length = lengthArg->ReadPrimitive<int>();
		if (length < 0)
			length = 0;

		if (start + length > static_cast<int>(value.size()))
			length = static_cast<int>(value.size()) - start;
		
		std::wstring result = value.substr(start, length);
		return ObjectInstance::FromValue(result);
	}
	else
	{
		std::wstring result = value.substr(start);
		return ObjectInstance::FromValue(result);
	}
}

static ObjectInstance* Contains(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* searchArg = arguments->TryFind(L"value");
	if (searchArg == nullptr)
		return ObjectInstance::FromValue(false);
	
	std::wstring search = searchArg->ReadPrimitive<std::wstring>();
	return ObjectInstance::FromValue(value.find(search) != std::wstring::npos);
}

static ObjectInstance* StartsWith(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* prefixArg = arguments->TryFind(L"value");
	if (prefixArg == nullptr)
		return ObjectInstance::FromValue(false);
	
	std::wstring prefix = prefixArg->ReadPrimitive<std::wstring>();
	if (prefix.size() > value.size())
		return ObjectInstance::FromValue(false);
	
	return ObjectInstance::FromValue(value.substr(0, prefix.size()) == prefix);
}

static ObjectInstance* EndsWith(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* suffixArg = arguments->TryFind(L"value");
	if (suffixArg == nullptr)
		return ObjectInstance::FromValue(false);
	
	std::wstring suffix = suffixArg->ReadPrimitive<std::wstring>();
	if (suffix.size() > value.size())
		return ObjectInstance::FromValue(false);
	
	return ObjectInstance::FromValue(value.substr(value.size() - suffix.size()) == suffix);
}

static ObjectInstance* IndexOf(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* searchArg = arguments->TryFind(L"value");
	if (searchArg == nullptr)
		return ObjectInstance::FromValue(-1);
	
	std::wstring search = searchArg->ReadPrimitive<std::wstring>();
	size_t pos = value.find(search);
	return ObjectInstance::FromValue(pos == std::wstring::npos ? -1 : static_cast<int>(pos));
}

static ObjectInstance* LastIndexOf(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* searchArg = arguments->TryFind(L"value");
	if (searchArg == nullptr)
		return ObjectInstance::FromValue(-1);
	
	std::wstring search = searchArg->ReadPrimitive<std::wstring>();
	size_t pos = value.rfind(search);
	return ObjectInstance::FromValue(pos == std::wstring::npos ? -1 : static_cast<int>(pos));
}

static ObjectInstance* Replace(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* oldStrArg = arguments->TryFind(L"oldValue");
	ObjectInstance* newStrArg = arguments->TryFind(L"newValue");
	
	if (oldStrArg == nullptr || newStrArg == nullptr)
		return ObjectInstance::FromValue(value);
	
	std::wstring oldStr = oldStrArg->ReadPrimitive<std::wstring>();
	std::wstring newStr = newStrArg->ReadPrimitive<std::wstring>();
	
	std::wstring result = value;
	size_t pos = 0;

	while ((pos = result.find(oldStr, pos)) != std::wstring::npos)
	{
		result.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
	
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* ToUpper(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	std::wstring result = value;
	transform(result.begin(), result.end(), result.begin(), ::towupper);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* ToLower(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	std::wstring result = value;
	transform(result.begin(), result.end(), result.begin(), ::towlower);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Trim(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	// Trim from start
	size_t start = value.find_first_not_of(L" \t\n\r");
	if (start == std::wstring::npos)
		return ObjectInstance::FromValue(L"");
	
	// Trim from end
	size_t end = value.find_last_not_of(L" \t\n\r");
	
	std::wstring result = value.substr(start, end - start + 1);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Format(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this"); // string
	ObjectInstance* formatArgs = arguments->TryFind(L"args"); // string[]

	std::wstring format = instance->ReadPrimitive<std::wstring>();
	std::wstring result = format;
	size_t pos = 0;
	
	// Simple format: replace {0}, {1}, etc. with arguments
	// Check for arguments arg0, arg1, ..., arg9
	while ((pos = result.find(L"{", pos)) != std::wstring::npos)
	{
		size_t endPos = result.find(L"}", pos);
		if (endPos == std::wstring::npos)
			break;
		
		std::wstring placeholder = result.substr(pos + 1, endPos - pos - 1);
		int index = -1;
		
		try
		{
			index = stoi(placeholder);
		}
		catch (...)
		{
			pos = endPos + 1;
			continue;
		}
		
		/*
		// Find argument by index (arg0, arg1, etc.)
		std::wstring argName = L"arg" + ;
		ObjectInstance* arg = arguments->TryFind(argName);
		*/
		
		if (!formatArgs->IsInBounds(index))
			throw std::runtime_error("index is out of bounds");

		ObjectInstance* arg = formatArgs->GetElement(index);
		if (arg != nullptr)
		{
			// Try to convert argument to string by calling ToString method
			std::wstring argStr;
			
			// Try to find ToString method in the argument's type
			std::wstring toStringName = L"ToString";
			std::vector<TypeSymbol*> emptyParams; // ToString has no parameters
			MethodSymbol* toStringMethod = const_cast<TypeSymbol*>(arg->Info)->FindMethod(toStringName, emptyParams);
			
			if (toStringMethod != nullptr)
			{
				try
				{
					InboundVariablesContext* toStringArgs = new InboundVariablesContext(nullptr);
					toStringArgs->SetVariable(L"this", arg);
					ObjectInstance* toStringResult = AbstractInterpreter::ExecuteMethod(toStringMethod, toStringArgs);
					
					if (toStringResult != nullptr && toStringResult->Info == SymbolTable::Primitives::String)
					{
						argStr = toStringResult->ReadPrimitive<std::wstring>();
					}
					else
					{
						argStr = L"<toString error>";
					}
					
					// Clean up
					delete toStringArgs;
				}
				catch (...)
				{
					argStr = L"<toString exception>";
				}
			}
			else
			{
				// Fallback to primitive type conversion if ToString is not available
				if (arg->Info == SymbolTable::Primitives::String)
				{
					argStr = arg->ReadPrimitive<std::wstring>();
				}
				else if (arg->Info == SymbolTable::Primitives::Integer)
				{
					argStr = std::to_wstring(arg->ReadPrimitive<int>());
				}
				else if (arg->Info == SymbolTable::Primitives::Boolean)
				{
					argStr = arg->ReadPrimitive<bool>() ? L"true" : L"false";
				}
				else if (arg->Info == SymbolTable::Primitives::Char)
				{
					argStr = std::wstring(1, arg->ReadPrimitive<wchar_t>());
				}
				else
				{
					argStr = L"<unknown>";
				}
			}
			
			result.replace(pos, endPos - pos + 1, argStr);
			pos += argStr.length();
		}
		else
		{
			// Argument not found, keep placeholder
			pos = endPos + 1;
		}
	}
	
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* PadLeft(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* widthArg = arguments->TryFind(L"width");
	if (widthArg == nullptr)
		return ObjectInstance::FromValue(value);
	
	int width = widthArg->ReadPrimitive<int>();
	if (width <= static_cast<int>(value.size()))
		return ObjectInstance::FromValue(value);
	
	wchar_t padChar = L' ';
	ObjectInstance* padCharArg = arguments->TryFind(L"padChar");
	if (padCharArg != nullptr)
	{
		padChar = padCharArg->ReadPrimitive<wchar_t>();
	}
	
	std::wstring result = std::wstring(width - value.size(), padChar) + value;
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* PadRight(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* widthArg = arguments->TryFind(L"width");
	if (widthArg == nullptr)
		return ObjectInstance::FromValue(value);
	
	int width = widthArg->ReadPrimitive<int>();
	if (width <= static_cast<int>(value.size()))
		return ObjectInstance::FromValue(value);
	
	wchar_t padChar = L' ';
	ObjectInstance* padCharArg = arguments->TryFind(L"padChar");
	if (padCharArg != nullptr)
	{
		padChar = padCharArg->ReadPrimitive<wchar_t>();
	}
	
	std::wstring result = value + std::wstring(width - value.size(), padChar);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Remove(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* startArg = arguments->TryFind(L"start");
	if (startArg == nullptr)
		return ObjectInstance::FromValue(value);
	
	int start = startArg->ReadPrimitive<int>();
	if (start < 0)
		start = 0;
	if (start >= static_cast<int>(value.size()))
		return ObjectInstance::FromValue(value);
	
	ObjectInstance* countArg = arguments->TryFind(L"count");
	if (countArg != nullptr)
	{
		int count = countArg->ReadPrimitive<int>();
		if (count < 0)
			count = 0;
		if (start + count > static_cast<int>(value.size()))
			count = static_cast<int>(value.size()) - start;
		
		std::wstring result = value;
		result.erase(start, count);
		return ObjectInstance::FromValue(result);
	}
	else
	{
		std::wstring result = value.substr(0, start);
		return ObjectInstance::FromValue(result);
	}
}

static ObjectInstance* Insert(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	std::wstring value = instance->ReadPrimitive<std::wstring>();
	
	ObjectInstance* startArg = arguments->TryFind(L"start");
	ObjectInstance* strArg = arguments->TryFind(L"value");
	
	if (startArg == nullptr || strArg == nullptr)
		return ObjectInstance::FromValue(value);
	
	int start = startArg->ReadPrimitive<int>();
	std::wstring str = strArg->ReadPrimitive<std::wstring>();
	
	if (start < 0)
		start = 0;

	if (start > static_cast<int>(value.size()))
		start = static_cast<int>(value.size());
	
	std::wstring result = value;
	result.insert(start, str);
	return ObjectInstance::FromValue(result);
}

void StringPrimitive::Reflect(TypeSymbol* symbol)
{
	// IsEmpty
	MethodSymbol* isEmpty = new MethodSymbol(L"IsEmpty", IsEmpty);
	isEmpty->Accesibility = SymbolAccesibility::Public;
	isEmpty->ReturnType = SymbolTable::Primitives::Boolean;
	isEmpty->IsStatic = false;
	symbol->Methods.push_back(isEmpty);
	
	// GetLength
	MethodSymbol* getLength = new MethodSymbol(L"GetLength", GetLength);
	getLength->Accesibility = SymbolAccesibility::Public;
	getLength->ReturnType = SymbolTable::Primitives::Integer;
	getLength->IsStatic = false;
	symbol->Methods.push_back(getLength);
	
	// Substring(start, length?)
	MethodSymbol* substring = new MethodSymbol(L"Substring", Substring);
	substring->Accesibility = SymbolAccesibility::Public;
	substring->ReturnType = SymbolTable::Primitives::String;
	substring->IsStatic = false;
	ParameterSymbol* startParam = new ParameterSymbol(L"start");
	startParam->Type = SymbolTable::Primitives::Integer;
	substring->Parameters.push_back(startParam);
	ParameterSymbol* lengthParam = new ParameterSymbol(L"length");
	lengthParam->Type = SymbolTable::Primitives::Integer;
	substring->Parameters.push_back(lengthParam);
	symbol->Methods.push_back(substring);
	
	// Contains(value)
	MethodSymbol* contains = new MethodSymbol(L"Contains", Contains);
	contains->Accesibility = SymbolAccesibility::Public;
	contains->ReturnType = SymbolTable::Primitives::Boolean;
	contains->IsStatic = false;
	ParameterSymbol* containsParam = new ParameterSymbol(L"value");
	containsParam->Type = SymbolTable::Primitives::String;
	contains->Parameters.push_back(containsParam);
	symbol->Methods.push_back(contains);
	
	// StartsWith(value)
	MethodSymbol* startsWith = new MethodSymbol(L"StartsWith", StartsWith);
	startsWith->Accesibility = SymbolAccesibility::Public;
	startsWith->ReturnType = SymbolTable::Primitives::Boolean;
	startsWith->IsStatic = false;
	ParameterSymbol* startsWithParam = new ParameterSymbol(L"value");
	startsWithParam->Type = SymbolTable::Primitives::String;
	startsWith->Parameters.push_back(startsWithParam);
	symbol->Methods.push_back(startsWith);
	
	// EndsWith(value)
	MethodSymbol* endsWith = new MethodSymbol(L"EndsWith", EndsWith);
	endsWith->Accesibility = SymbolAccesibility::Public;
	endsWith->ReturnType = SymbolTable::Primitives::Boolean;
	endsWith->IsStatic = false;
	ParameterSymbol* endsWithParam = new ParameterSymbol(L"value");
	endsWithParam->Type = SymbolTable::Primitives::String;
	endsWith->Parameters.push_back(endsWithParam);
	symbol->Methods.push_back(endsWith);
	
	// IndexOf(value)
	MethodSymbol* indexOf = new MethodSymbol(L"IndexOf", IndexOf);
	indexOf->Accesibility = SymbolAccesibility::Public;
	indexOf->ReturnType = SymbolTable::Primitives::Integer;
	indexOf->IsStatic = false;
	ParameterSymbol* indexOfParam = new ParameterSymbol(L"value");
	indexOfParam->Type = SymbolTable::Primitives::String;
	indexOf->Parameters.push_back(indexOfParam);
	symbol->Methods.push_back(indexOf);
	
	// LastIndexOf(value)
	MethodSymbol* lastIndexOf = new MethodSymbol(L"LastIndexOf", LastIndexOf);
	lastIndexOf->Accesibility = SymbolAccesibility::Public;
	lastIndexOf->ReturnType = SymbolTable::Primitives::Integer;
	lastIndexOf->IsStatic = false;
	ParameterSymbol* lastIndexOfParam = new ParameterSymbol(L"value");
	lastIndexOfParam->Type = SymbolTable::Primitives::String;
	lastIndexOf->Parameters.push_back(lastIndexOfParam);
	symbol->Methods.push_back(lastIndexOf);
	
	// Replace(oldValue, newValue)
	MethodSymbol* replace = new MethodSymbol(L"Replace", Replace);
	replace->Accesibility = SymbolAccesibility::Public;
	replace->ReturnType = SymbolTable::Primitives::String;
	replace->IsStatic = false;
	ParameterSymbol* oldValueParam = new ParameterSymbol(L"oldValue");
	oldValueParam->Type = SymbolTable::Primitives::String;
	replace->Parameters.push_back(oldValueParam);
	ParameterSymbol* newValueParam = new ParameterSymbol(L"newValue");
	newValueParam->Type = SymbolTable::Primitives::String;
	replace->Parameters.push_back(newValueParam);
	symbol->Methods.push_back(replace);
	
	// ToUpper()
	MethodSymbol* toUpper = new MethodSymbol(L"ToUpper", ToUpper);
	toUpper->Accesibility = SymbolAccesibility::Public;
	toUpper->ReturnType = SymbolTable::Primitives::String;
	toUpper->IsStatic = false;
	symbol->Methods.push_back(toUpper);
	
	// ToLower()
	MethodSymbol* toLower = new MethodSymbol(L"ToLower", ToLower);
	toLower->Accesibility = SymbolAccesibility::Public;
	toLower->ReturnType = SymbolTable::Primitives::String;
	toLower->IsStatic = false;
	symbol->Methods.push_back(toLower);
	
	// Trim()
	MethodSymbol* trim = new MethodSymbol(L"Trim", Trim);
	trim->Accesibility = SymbolAccesibility::Public;
	trim->ReturnType = SymbolTable::Primitives::String;
	trim->IsStatic = false;
	symbol->Methods.push_back(trim);
	
	// Format - supports up to 10 arguments (arg0-arg9)
	MethodSymbol* format = new MethodSymbol(L"Format", Format);
	format->Accesibility = SymbolAccesibility::Public;
	format->ReturnType = SymbolTable::Primitives::String;
	format->IsStatic = false;

	/*
	// Add optional parameters for Format: arg0, arg1, ..., arg9
	for (int i = 0; i < 10; i++)
	{
		std::wstring paramName = L"arg" + std::to_wstring(i);
		ParameterSymbol* formatParam = new ParameterSymbol(paramName);
		formatParam->Type = SymbolTable::Primitives::String; // Can be any type, but default to String
		format->Parameters.push_back(formatParam);
	}
	*/

	std::wstring paramName = L"args";
	ParameterSymbol* formatParam = new ParameterSymbol(paramName);
	ArrayTypeSymbol* formatParamType = new ArrayTypeSymbol(SymbolTable::Primitives::String);
	formatParam->Type = formatParamType;
	format->Parameters.push_back(formatParam);
	symbol->Methods.push_back(format);
	
	// PadLeft(width, padChar?)
	MethodSymbol* padLeft = new MethodSymbol(L"PadLeft", PadLeft);
	padLeft->Accesibility = SymbolAccesibility::Public;
	padLeft->ReturnType = SymbolTable::Primitives::String;
	padLeft->IsStatic = false;
	ParameterSymbol* padLeftWidth = new ParameterSymbol(L"width");
	padLeftWidth->Type = SymbolTable::Primitives::Integer;
	padLeft->Parameters.push_back(padLeftWidth);
	ParameterSymbol* padLeftChar = new ParameterSymbol(L"padChar");
	padLeftChar->Type = SymbolTable::Primitives::Char;
	padLeft->Parameters.push_back(padLeftChar);
	symbol->Methods.push_back(padLeft);
	
	// PadRight(width, padChar?)
	MethodSymbol* padRight = new MethodSymbol(L"PadRight", PadRight);
	padRight->Accesibility = SymbolAccesibility::Public;
	padRight->ReturnType = SymbolTable::Primitives::String;
	padRight->IsStatic = false;
	ParameterSymbol* padRightWidth = new ParameterSymbol(L"width");
	padRightWidth->Type = SymbolTable::Primitives::Integer;
	padRight->Parameters.push_back(padRightWidth);
	ParameterSymbol* padRightChar = new ParameterSymbol(L"padChar");
	padRightChar->Type = SymbolTable::Primitives::Char;
	padRight->Parameters.push_back(padRightChar);
	symbol->Methods.push_back(padRight);
	
	// Remove(start, count?)
	MethodSymbol* remove = new MethodSymbol(L"Remove", Remove);
	remove->Accesibility = SymbolAccesibility::Public;
	remove->ReturnType = SymbolTable::Primitives::String;
	remove->IsStatic = false;
	ParameterSymbol* removeStart = new ParameterSymbol(L"start");
	removeStart->Type = SymbolTable::Primitives::Integer;
	remove->Parameters.push_back(removeStart);
	ParameterSymbol* removeCount = new ParameterSymbol(L"count");
	removeCount->Type = SymbolTable::Primitives::Integer;
	remove->Parameters.push_back(removeCount);
	symbol->Methods.push_back(remove);
	
	// Insert(start, value)
	MethodSymbol* insert = new MethodSymbol(L"Insert", Insert);
	insert->Accesibility = SymbolAccesibility::Public;
	insert->ReturnType = SymbolTable::Primitives::String;
	insert->IsStatic = false;
	ParameterSymbol* insertStart = new ParameterSymbol(L"start");
	insertStart->Type = SymbolTable::Primitives::Integer;
	insert->Parameters.push_back(insertStart);
	ParameterSymbol* insertValue = new ParameterSymbol(L"value");
	insertValue->Type = SymbolTable::Primitives::String;
	insert->Parameters.push_back(insertValue);
	symbol->Methods.push_back(insert);
}
