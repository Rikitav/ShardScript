#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <cstring>
#include <stdexcept>
#include <string>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax::symbols;

ObjectInstance* ObjectInstance::Copy()
{
	if (Info->IsReferenceType)
	{
		IncrementReference();
		return this;
	}

	ObjectInstance* newInstance = GarbageCollector::AllocateInstance(Info);
	memcpy((void*)newInstance->Ptr, Ptr, Info->MemoryBytesSize);
	return newInstance;
}

ObjectInstance* ObjectInstance::GetField(FieldSymbol* field)
{
	void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(void*));
	return GarbageCollector::CopyInstance(field->ReturnType, offset);

	if (field->ReturnType->IsReferenceType)
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(void*));
		void* memory = *static_cast<void**>(offset);
		return GarbageCollector::CopyInstance(field->ReturnType, memory);
	}
	else
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, field->ReturnType->MemoryBytesSize);
		return GarbageCollector::CopyInstance(field->ReturnType, offset);
	}
}

void ObjectInstance::SetField(FieldSymbol* field, ObjectInstance* instance)
{
	if (field->ReturnType->IsReferenceType)
	{
		if (instance == nullptr)
		{
			void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(void*));
			memset(offset, 0, sizeof(void*));
			return;
		}

		ObjectInstance* oldValue = GetField(field);
		oldValue->DecrementReference();
		
		instance->IncrementReference();
		WriteMemory(field->MemoryBytesOffset, sizeof(void*), &instance->Ptr);
	}
	else
	{
		if (instance == nullptr)
			throw runtime_error("cannot write null value to ValueType field");

		WriteMemory(field->MemoryBytesOffset, field->ReturnType->MemoryBytesSize, instance->Ptr);
	}
}

void ObjectInstance::CopyTo(ObjectInstance* to)
{
	if (Info != to->Info)
		throw runtime_error("cannot copy instance memory of different types");

	to->WriteMemory(0, Info->MemoryBytesSize, Ptr);
}

unsigned long ObjectInstance::GetReferencesCount()
{
	return *static_cast<long*>(OffsetMemory(0, sizeof(long)));
}

void ObjectInstance::IncrementReference()
{
	unsigned long* refCounter = static_cast<unsigned long*>(OffsetMemory(0, sizeof(unsigned long)));
	refCounter[0] += 1;
}

void ObjectInstance::DecrementReference()
{
	unsigned long* refCounter = static_cast<unsigned long*>(OffsetMemory(0, sizeof(unsigned long)));
	refCounter[0] -= 1;
}

void* ObjectInstance::OffsetMemory(const size_t offset, const size_t size)
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	return (void*)(static_cast<const char*>(Ptr) + offset);
}

void ObjectInstance::ReadMemory(const size_t offset, const size_t size, const void* dst)
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	const char* memOffset = static_cast<const char*>(Ptr) + offset;
	memcpy((void*)dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const size_t offset, const size_t size, const void* src)
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	const char* memOffset = static_cast<const char*>(Ptr) + offset;
	memcpy((void*)memOffset, src, size);
}
