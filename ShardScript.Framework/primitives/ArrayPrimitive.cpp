#include <ShardScript.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

#include "PrimitivesLoading.hpp"

using namespace shard;

static ObjectInstance* get_Length(const CallState& context)
{
	static FieldSymbol* lengthField = SymbolTable::Primitives::Array->Fields.at(0); // <Length>k__BackingField

	ObjectInstance* instance = context.Args[0];
	ObjectInstance* lengthInstance = instance->GetField(lengthField);
	return lengthInstance;
}

static ObjectInstance* get_Item(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	ObjectInstance* index = context.Args[1]; // index

	int64_t indexValue = index->AsInteger();
	if (!instance->IsInBounds(indexValue))
		throw std::runtime_error("index is out of bounds");

	return instance->GetElement(indexValue);
}

static ObjectInstance* set_Item(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	ObjectInstance* index = context.Args[1]; // index
	ObjectInstance* value = context.Args[2]; // value

	int64_t indexValue = index->AsInteger();
	if (!instance->IsInBounds(indexValue))
		throw std::runtime_error("index is out of bounds");

	instance->SetElement(indexValue, value);
	return nullptr;
}

static std::wstring ObjectInstanceToString(const CallState& context, ObjectInstance* instance)
{
	static std::wstring findName = L"ToString";
	static std::vector<TypeSymbol*> findArgs;

	TypeSymbol* info = const_cast<TypeSymbol*>(instance->Info);
	MethodSymbol* toString = info->FindMethod(findName, findArgs);

	if (toString == nullptr)
		return info->FullName;

	context.Runtimer.InvokeMethod(toString, { instance });
	CallStackFrame* currentFrame = context.Runtimer.CurrentFrame();

	ObjectInstance* resultInstance = currentFrame->PopStack();
	std::wstring resultStr = resultInstance->AsString();

	context.Collector.CollectInstance(resultInstance);
	return resultStr;
}

static ObjectInstance* to_string(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->Info);
	size_t size = array->Size;

	if (size == 0)
		return context.Collector.FromValue(L"[]");

	std::wostringstream result;
	result << L"[";

	ObjectInstance* element = instance->GetElement(0);
	result << ObjectInstanceToString(context, element);
	context.Collector.CollectInstance(element);

	for (size_t i = 1; i < size; i++)
	{
		element = instance->GetElement(i);
		result << L", " << ObjectInstanceToString(context, element);
		context.Collector.CollectInstance(element);
	}

	result << L"]";
	return context.Collector.FromValue(result.str());
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
