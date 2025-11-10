#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <malloc.h>
#include <stdexcept>
#include <cstring>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::runtime;

ObjectInstance* GarbageCollector::NullInstance = new ObjectInstance(-1, nullptr, nullptr);

ObjectInstance* GarbageCollector::GetStaticField(FieldSymbol* field)
{
	ObjectInstance* staticFieldInstance = nullptr;
	auto find = staticFields.find(field);
	if (find == staticFields.end())
	{
		staticFieldInstance = field->ReturnType->IsReferenceType
			? NullInstance
			: GarbageCollector::AllocateInstance(field->ReturnType);

		staticFields[field] = staticFieldInstance;
	}
	else
	{
		staticFieldInstance = find->second;
	}

	return CopyInstance(staticFieldInstance);
}

void GarbageCollector::SetStaticField(FieldSymbol* field, ObjectInstance* instance)
{
	if (field->ReturnType->IsReferenceType)
	{
		ObjectInstance* oldValue = GetStaticField(field);
		GarbageCollector::DestroyInstance(oldValue);
	}

	staticFields[field] = CopyInstance(instance);
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
{
	void* memory = malloc(objectInfo->MemoryBytesSize);
	if (memory == nullptr)
		throw runtime_error("cannot allocate memory for new instance");

	memset(memory, 0, objectInfo->MemoryBytesSize);
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, memory);
	instance->IncrementReference();

	Heap.add(instance);
	return instance;
}

/*
ObjectInstance* GarbageCollector::CreateInstance(const TypeSymbol* objectInfo, void* ptr)
{
	ObjectInstance* newInstance = new ObjectInstance(objectsCounter++, objectInfo, , ptr);
	return newInstance;
}
*/

ObjectInstance* GarbageCollector::CopyInstance(const TypeSymbol* objectInfo, void* ptr)
{
	void* memory = malloc(objectInfo->MemoryBytesSize);
	if (ptr == nullptr)
		throw runtime_error("cannot allocate memory for new instance");

	memcpy(memory, ptr, objectInfo->MemoryBytesSize);
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, ptr);
	instance->IncrementReference();

	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance == NullInstance)
		return instance;

	if (instance->Info->IsReferenceType)
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(instance->Info);
	newInstance->WriteMemory(0, instance->Info->MemoryBytesSize, instance->Ptr);
	return newInstance;
}

void GarbageCollector::CopyInstance(ObjectInstance* from, ObjectInstance* to)
{
	if (from->Info != to->Info)
		throw runtime_error("cannot copy instance memory of different types");

	to->WriteMemory(0, to->Info->MemoryBytesSize, from->Ptr);
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		throw runtime_error("requested destroying nullptr");

	if (instance->Info->IsReferenceType)
	{
		instance->DecrementReference();
		if (instance->ReferencesCounter > 0)
			return;
	}

	TerminateInstance(instance);
}

void GarbageCollector::TerminateInstance(ObjectInstance* instance)
{
	for (FieldSymbol* field : instance->Info->Fields)
	{
		if (field->ReturnType->IsReferenceType)
			DestroyInstance(instance->GetField(field));
	}

	Heap.erase(instance);
	free(instance->Ptr);
	instance->~ObjectInstance();
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
