#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <malloc.h>
#include <stdexcept>
#include <cstring>

using namespace std;
using namespace shard::syntax::symbols;
using namespace shard::runtime;

ObjectInstance* GarbageCollector::NullInstance(const TypeSymbol* objectInfo)
{
	auto find = nullInstancesMap.find(const_cast<TypeSymbol*>(objectInfo));
	ObjectInstance* instance = nullptr;
	
	if (find == nullInstancesMap.end())
		instance = new ObjectInstance(-1, objectInfo, nullptr);

	return nullptr;
}

ObjectInstance* GarbageCollector::AllocateInstance(const TypeSymbol* objectInfo)
{
	void* ptr = malloc(objectInfo->MemoryBytesSize);
	if (ptr == nullptr)
		throw runtime_error("cannot allocate memory for new instance");

	memset(ptr, 0, objectInfo->MemoryBytesSize);
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, ptr);
	instance->IncrementReference();

	for (FieldSymbol* field : objectInfo->Fields)
	{
		/*
		if (field->ReturnType->IsReferenceType)
		{
			instance->SetField(field, nullptr);
		}
		else
		{
			//instance->SetField(field)
		}
		*/
	}

	Heap.add(instance);
	return instance;
}

ObjectInstance* GarbageCollector::CreateInstance(const TypeSymbol* objectInfo, void* ptr)
{
	ObjectInstance* newInstance = new ObjectInstance(objectsCounter++, objectInfo, ptr);
	return newInstance;
}

ObjectInstance* GarbageCollector::CopyInstance(const TypeSymbol* objectInfo, void* ptr)
{
	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(objectInfo);
	memcpy(newInstance->Ptr, ptr, objectInfo->MemoryBytesSize);
	return newInstance;
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* from, ObjectInstance* to)
{
	if (from->Info != to->Info)
		throw runtime_error("cannot copy instance memory of different types");

	to->WriteMemory(0, to->Info->MemoryBytesSize, from->Ptr);
}

ObjectInstance* GarbageCollector::CopyInstance(ObjectInstance* instance)
{
	if (instance->Info->IsReferenceType)
	{
		instance->IncrementReference();
		return instance;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(instance->Info);
	newInstance->WriteMemory(0, instance->Info->MemoryBytesSize, instance->Ptr);
	return newInstance;
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance == nullptr)
		return;

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
	free((void*)instance->Ptr);
	instance->~ObjectInstance();
}

void GarbageCollector::Terminate()
{
	for (ObjectInstance* instance : Heap)
		TerminateInstance(instance);
}
