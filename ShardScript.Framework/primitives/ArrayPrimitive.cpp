#include <shard/syntax/SymbolAccesibility.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/CallStackFrame.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

#include "PrimitivesLoading.h"

using namespace shard;

static ObjectInstance* get_Length(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	static FieldSymbol* lengthField = SymbolTable::Primitives::Array->Fields.at(0); // <Length>k__BackingField

	ObjectInstance* instance = arguments[0];
	ObjectInstance* lengthInstance = instance->GetField(lengthField);
	return lengthInstance;
}

static ObjectInstance* get_Item(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	ObjectInstance* index = arguments[1]; // index

	int64_t indexValue = index->AsInteger();
	if (!instance->IsInBounds(indexValue))
		throw std::runtime_error("index is out of bounds");

	return instance->GetElement(indexValue);
}

static ObjectInstance* set_Item(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	ObjectInstance* index = arguments[1]; // index
	ObjectInstance* value = arguments[2]; // value

	int64_t indexValue = index->AsInteger();
	if (!instance->IsInBounds(indexValue))
		throw std::runtime_error("index is out of bounds");

	instance->SetElement(indexValue, value);
	return nullptr;
}

static std::wstring ObjectInstanceToString(const VirtualMachine* host, ObjectInstance* instance)
{
	static std::wstring findName = L"ToString";
	static std::vector<TypeSymbol*> findArgs;

	TypeSymbol* info = const_cast<TypeSymbol*>(instance->Info);
	MethodSymbol* toString = info->FindMethod(findName, findArgs);

	if (toString == nullptr)
		return info->FullName;

	host->InvokeMethod(toString, { instance });
	CallStackFrame* currentFrame = host->CurrentFrame();

	ObjectInstance* resultInstance = currentFrame->PopStack();
	std::wstring resultStr = resultInstance->AsString();

	GarbageCollector::CollectInstance(resultInstance);
	return resultStr;
}

static ObjectInstance* to_string(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->Info);
	size_t size = array->Size;

	if (size == 0)
		return ObjectInstance::FromValue(L"[]");

	std::wostringstream result;
	result << L"[";

	ObjectInstance* element = instance->GetElement(0);
	result << ObjectInstanceToString(host, element);
	GarbageCollector::CollectInstance(element);

	for (size_t i = 1; i < size; i++)
	{
		element = instance->GetElement(i);
		result << L", " << ObjectInstanceToString(host, element);
		GarbageCollector::CollectInstance(element);
	}

	result << L"]";
	return ObjectInstance::FromValue(result.str());
}

void ArrayPrimitive::Reflect(TypeSymbol* symbol)
{
	// Length
	{
		PropertySymbol* lengthProperty = new PropertySymbol(std::wstring(L"Length"));
		lengthProperty->Accesibility = SymbolAccesibility::Public;
		lengthProperty->ReturnType = SymbolTable::Primitives::Integer;

		lengthProperty->GenerateBackingField();
		FieldSymbol* lenthField = lengthProperty->BackingField;
		symbol->Fields.push_back(lenthField);

		lengthProperty->Getter = new AccessorSymbol(L"get_Length", get_Length);
		symbol->Properties.push_back(lengthProperty);
	}

	// indexator
	{
		IndexatorSymbol* indexatorSymbol = new IndexatorSymbol(std::wstring(L"Item"));
		indexatorSymbol->Accesibility = SymbolAccesibility::Public;
		indexatorSymbol->IsStatic = false;

		ParameterSymbol* indexParam = new ParameterSymbol(L"index");
		indexParam->Type = SymbolTable::Primitives::Integer;
		indexatorSymbol->Parameters.push_back(indexParam);

		{
			AccessorSymbol* getter = new AccessorSymbol(L"get_Item", get_Item);
			getter->Parameters.push_back(new ParameterSymbol(L"index", SymbolTable::Primitives::Integer));
			indexatorSymbol->Getter = getter;
		}

		{
			AccessorSymbol* setter = new AccessorSymbol(L"set_Item", set_Item);
			setter->Parameters.push_back(new ParameterSymbol(L"index", SymbolTable::Primitives::Integer));
			indexatorSymbol->Setter = setter;
		}

		symbol->Indexators.push_back(indexatorSymbol);
	}

	// ToString
	{
		MethodSymbol* toStringMethod = new MethodSymbol(L"ToString", to_string);
		toStringMethod->Accesibility = SymbolAccesibility::Public;
		toStringMethod->ReturnType = SymbolTable::Primitives::String;

		symbol->Methods.push_back(toStringMethod);
	}
}
