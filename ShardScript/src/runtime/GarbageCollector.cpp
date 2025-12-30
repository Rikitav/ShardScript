#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/AbstractInterpreter.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <malloc.h>
#include <stdexcept>
#include <cstring>

using namespace shard;

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(-1, nullptr, nullptr);

ObjectInstance* GarbageCollector::GetStaticField(FieldSymbol* field)
{
	if (auto find = staticFields.find(field); find != staticFields.end())
		return find->second;

	if (field->DefaultValueExpression != nullptr)
	{
		ObjectInstance* staticFieldInstance = AbstractInterpreter::EvaluateExpression(field->DefaultValueExpression);
		staticFields[field] = staticFieldInstance;
		return staticFieldInstance;
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

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
{
	if (objectInfo == nullptr)
		throw std::runtime_error("objectInfo is nullptr");

	/*
	if (objectInfo->MemoryBytesSize == 0)
		throw std::runtime_error("tried to allocate object instance of size 0");
	*/

	void* memory = malloc(objectInfo->MemoryBytesSize);
	if (memory == nullptr)
		throw std::runtime_error("cannot allocate memory for new instance");

	memset(memory, 0, objectInfo->MemoryBytesSize);
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, memory);

	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(const TypeSymbol* objectInfo, void* ptr)
{
	if (ptr == nullptr)
		throw std::runtime_error("requested copying from nullptr");

	void* memory = malloc(objectInfo->MemoryBytesSize);
	if (memory == nullptr)
		throw std::runtime_error("cannot allocate memory for new instance");

	memcpy(memory, ptr, objectInfo->MemoryBytesSize);
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, ptr);

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
	newInstance->WriteMemory(0, instance->Info->MemoryBytesSize, instance->Ptr);
	newInstance->IncrementReference();
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

	if (!instance->IsFieldInstance)
		free(instance->Ptr);

	Heap.erase(instance);
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
