#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <cstring>
#include <stdexcept>
#include <string>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax::symbols;

ObjectInstance* ObjectInstance::GetField(FieldSymbol* field)
{
	if (field->ReturnType->IsReferenceType)
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*));
		void* valuePtr = *static_cast<void**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : static_cast<ObjectInstance*>(valuePtr);
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
		GarbageCollector::DestroyInstance(oldValue);
		
		instance->IncrementReference();
		WriteMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == nullptr)
			throw runtime_error("cannot write null value to ValueType field");

		WriteMemory(field->MemoryBytesOffset, field->ReturnType->MemoryBytesSize, instance->Ptr);
	}
}

void ObjectInstance::IncrementReference()
{
	if (!Info->IsReferenceType)
		return;

	ReferencesCounter += 1;
}

void ObjectInstance::DecrementReference()
{
	if (!Info->IsReferenceType)
		return;

	ReferencesCounter -= 1;
}

void* ObjectInstance::OffsetMemory(const size_t offset, const size_t size)
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	return static_cast<char*>(Ptr) + offset;
}

void ObjectInstance::ReadMemory(const size_t offset, const size_t size, void* dst)
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	const char* memOffset = static_cast<char*>(Ptr) + offset;
	memcpy(dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const size_t offset, const size_t size, void* src)
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw out_of_range("offset (" + to_string(offset) + ") + size (" + to_string(size) + ") is out of instance's memory range (" + to_string(Info->MemoryBytesSize) + ").");

	char* memOffset = static_cast<char*>(Ptr) + offset;
	memcpy(memOffset, src, size);
}
