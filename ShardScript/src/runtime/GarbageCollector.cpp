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

ObjectInstance* GarbageCollector::CopyInstance(const TypeSymbol* objectInfo, void* ptr)
{
	ObjectInstance* instance = new ObjectInstance(objectsCounter++, objectInfo, ptr);
	return instance;
}

void GarbageCollector::DestroyInstance(ObjectInstance* instance)
{
	if (instance->Info->IsReferenceType)
	{
		instance->DecrementReference();
		if (instance->GetReferencesCount() > 0)
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
