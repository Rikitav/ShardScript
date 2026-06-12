#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <malloc.h>
#include <stdexcept>
#include <cstring>
#include <wchar.h>
#include <cstdint>
#include <string>

using namespace shard;

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(nullptr, nullptr, true);

GarbageCollector::GarbageCollector(ApplicationDomain* domain) : applicationDomain(domain)
{

}

ObjectInstance* GarbageCollector::FromValue(bool value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
	instance->WriteBoolean(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(int64_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	instance->WriteInteger(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(double value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Double);
	instance->WriteDouble(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(wchar_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
	instance->WriteCharacter(value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(const wchar_t* value, bool isTransient)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String, isTransient);

	instance->WriteInteger(wcslen(value));
	instance->WriteMemory(sizeof(int64_t), sizeof(wchar_t*), &value);
	return instance;
}

ObjectInstance* GarbageCollector::FromValue(const std::wstring& value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String, false);

	size_t length = value.size();
	size_t size = length * sizeof(wchar_t) + sizeof(wchar_t);

	wchar_t* copy = static_cast<wchar_t*>(malloc(size));
	if (copy == nullptr)
		throw std::runtime_error("Failed to allocate string");

	wcscpy(copy, value.data());

	instance->WriteInteger(length);
	instance->WriteMemory(sizeof(int64_t), sizeof(wchar_t*), &copy);
	return instance;
}

ObjectInstance* GarbageCollector::GetStaticField(FieldSymbol* field)
{
	if (auto find = staticFields.find(field); find != staticFields.end())
		return find->second;

	if (field->DefaultValueExpression != nullptr)
	{
		// TODO: FIX!!!
		/*
		ObjectInstance* staticFieldInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		staticFields[field] = staticFieldInstance;
		return staticFieldInstance;
		*/
		return nullptr;
	}

	ObjectInstance* staticFieldInstance = !field->ReturnType->IsReferenceType
		? GarbageCollector::AllocateInstance(field->ReturnType)
		: NullInstance;

	staticFields[field] = staticFieldInstance;
	return staticFieldInstance;
}

void GarbageCollector::SetStaticField(FieldSymbol* field, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested setting static field to nullptr");

	if (auto find = staticFields.find(field); find != staticFields.end())
	{
		ObjectInstance* oldValue = GetStaticField(field);
		GarbageCollector::CollectInstance(oldValue);
	}

	staticFields[field] = CopyInstance(instance);
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo, bool isTransient)
{
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	if (objectInfo->State != TypeLayoutingState::Visited)
		throw std::runtime_error("objectInfo is uninitialized");

	void* rawMemory = nullptr;
	if (objectInfo->MemoryBytesSize > 0)
	{
		rawMemory = malloc(objectInfo->MemoryBytesSize);
		if (rawMemory == nullptr)
			throw std::runtime_error("cannot allocate memory for new instance");

		std::memset(rawMemory, 0, objectInfo->MemoryBytesSize);
	}

	ObjectInstance* instance = new ObjectInstance(objectInfo, rawMemory, isTransient);

	if (objectInfo->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(objectInfo);
		instance->SetArrayLength(arrayInfo->Size);
		instance->SetMemorySize(objectInfo->MemoryBytesSize);
	}

	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::AllocateArray(TypeSymbol* elementType, size_t length, bool isTransient)
{
	if (elementType == nullptr)
		throw std::runtime_error("elementType is nullptr");

	size_t headerSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	size_t elementSize = elementType->GetInlineSize();
	size_t totalSize = headerSize + elementSize * length;

	void* rawMemory = nullptr;
	if (totalSize > 0)
	{
		rawMemory = malloc(totalSize);
		if (rawMemory == nullptr)
			throw std::runtime_error("cannot allocate memory for dynamic array");

		std::memset(rawMemory, 0, totalSize);
	}

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(elementType);
	arrayType->Size = length;
	arrayType->MemoryBytesSize = totalSize;
	dynamicArrayTypes.emplace_back(arrayType);

	ObjectInstance* instance = new ObjectInstance(arrayType, rawMemory, isTransient);
	instance->SetArrayLength(length);
	instance->SetMemorySize(totalSize);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested copying nullptr");

	if (instance == NullInstance)
		return instance;

	if (instance->getInfo()->IsReferenceType)
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(instance->getInfo());
	newInstance->WriteMemory(0, instance->getInfo()->MemoryBytesSize, instance->getMemory());
	
	TypeSymbol* fieldOwner = const_cast<TypeSymbol*>(instance->getInfo());
	if (fieldOwner->Kind == SyntaxKind::GenericType)
		fieldOwner = static_cast<GenericTypeSymbol*>(fieldOwner)->UnderlayingType;

	for (FieldSymbol* field : fieldOwner->Fields)
	{
		if (field->ReturnType->IsReferenceType)
		{
			ObjectInstance* fieldValue = instance->GetField(field);
			fieldValue->IncrementReference();
		}
	}
	
	return newInstance;
}

void GarbageCollector::CollectInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	instance->DecrementReference();
	if (instance->getReferencesCounter() > 0)
		return;

	Heap.erase(instance);
	TerminateInstance(instance);
}

void GarbageCollector::TerminateInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested terminating nullptr");

	if (instance == NullInstance)
		return;

	TypeSymbol* fieldOwner = const_cast<TypeSymbol*>(instance->getInfo());
	if (fieldOwner->Kind == SyntaxKind::GenericType)
		fieldOwner = static_cast<GenericTypeSymbol*>(fieldOwner)->UnderlayingType;

	for (FieldSymbol* field : fieldOwner->Fields)
	{
		if (field->ReturnType->IsReferenceType)
			DestroyInstance(instance->GetField(field));
	}

	if (instance->getInfo()->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->getInfo());
		for (size_t i = 0; i < array->Size; i++)
		{
			ObjectInstance* element = instance->GetElement(i);
			DestroyInstance(element);
		}
	}

	if (!instance->getIsTransient())
	{
		if (instance->getInfo() == SymbolTable::Primitives::String)
		{
			void* stringPtr = instance->OffsetMemory(sizeof(int64_t), sizeof(wchar_t*));
			wchar_t* stringData = *static_cast<wchar_t**>(stringPtr);
			free(stringData);
		}

		free(instance->getMemory());
	}

	delete instance;
}

void GarbageCollector::Terminate()
{
	// Destroy all static instances
	for (const auto& choise : staticFields)
		TerminateInstance(choise.second);

	// Destroy all regular instances
	for (ObjectInstance* instance : Heap)
		TerminateInstance(instance);
}
