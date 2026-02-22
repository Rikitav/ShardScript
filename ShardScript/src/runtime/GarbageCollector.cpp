#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>

#include <malloc.h>
#include <stdexcept>
#include <cstring>

using namespace shard;

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(nullptr, nullptr, true);

ObjectInstance* GarbageCollector::GetStaticField(const VirtualMachine* host, FieldSymbol* field)
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

void GarbageCollector::SetStaticField(const VirtualMachine* host, FieldSymbol* field, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested setting static field to nullptr");

	if (auto find = staticFields.find(field); find != staticFields.end())
	{
		ObjectInstance* oldValue = GetStaticField(host, field);
		GarbageCollector::CollectInstance(oldValue);
	}

	staticFields[field] = CopyInstance(instance);
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
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

		memset(rawMemory, 0, objectInfo->MemoryBytesSize);
	}

	ObjectInstance* instance = new ObjectInstance(objectInfo, rawMemory, false);
	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested copying nullptr");

	if (instance == NullInstance)
		return instance;

	if (instance->Info->IsReferenceType)
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(instance->Info);
	newInstance->WriteMemory(0, instance->Info->MemoryBytesSize, instance->GetObjectMemory());
	return newInstance;
}

void GarbageCollector::CollectInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	if (instance->ReferencesCounter > 0)
		return;

	TerminateInstance(instance);
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested destroying nullptr");

	if (instance == NullInstance)
		return;

	instance->DecrementReference();
	if (instance->ReferencesCounter > 0)
		return;

	TerminateInstance(instance);
	Heap.erase(instance);
}

void GarbageCollector::TerminateInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("requested terminating nullptr");

	if (instance == NullInstance)
		return;

	for (FieldSymbol* field : instance->Info->Fields)
	{
		if (field->ReturnType->IsReferenceType)
			DestroyInstance(instance->GetField(field));
	}

	if (instance->Info->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(instance->Info);
		for (size_t i = 0; i < array->Size; i++)
		{
			ObjectInstance* element = instance->GetElement(i);
			DestroyInstance(element);
		}
	}

	if (!instance->IsTransient)
	{
		if (instance->Info == SymbolTable::Primitives::String)
		{
			void* stringPtr = instance->OffsetMemory(sizeof(int64_t), sizeof(wchar_t*));
			wchar_t* stringData = *static_cast<wchar_t**>(stringPtr);
			free(stringData);
		}

		free(instance->Memory);
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
