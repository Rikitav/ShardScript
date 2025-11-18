#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <cstring>
#include <stdexcept>
#include <string>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

ObjectInstance* ObjectInstance::FromValue(bool value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(int value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(wchar_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
	instance->WritePrimitive(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(const wchar_t* value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
	wstring* copy = new wstring(value);
	instance->WritePrimitive<wstring>(*copy);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(const wstring& value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
	wstring* copy = new wstring(value);
	instance->WritePrimitive<wstring>(*copy);
	instance->DecrementReference();
	return instance;
}

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
	if (instance == nullptr)
		throw runtime_error("got nullptr instance");

	if (field->ReturnType->IsReferenceType)
	{
		if (instance == GarbageCollector::NullInstance)
		{
			void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(void*));
			memset(offset, 0, sizeof(void*));
			return;
		}

		ObjectInstance* oldValue = GetField(field);
		if (oldValue != nullptr)
			GarbageCollector::DestroyInstance(oldValue);
		
		instance->IncrementReference();
		WriteMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw runtime_error("cannot write null value to ValueType field");

		WriteMemory(field->MemoryBytesOffset, field->ReturnType->MemoryBytesSize, instance->Ptr);
	}
}

ObjectInstance* ObjectInstance::GetElement(size_t index)
{
	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;
	size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;

	if (type->IsReferenceType)
	{
		void* offset = OffsetMemory(memoryOffset, sizeof(ObjectInstance*));
		void* valuePtr = *static_cast<void**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : static_cast<ObjectInstance*>(valuePtr);
	}
	else
	{
		void* offset = OffsetMemory(memoryOffset, type->MemoryBytesSize);
		return GarbageCollector::CopyInstance(type, offset);
	}
}

void ObjectInstance::SetElement(size_t index, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw runtime_error("got nullptr instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;
	size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;

	if (type->IsReferenceType)
	{
		if (instance == GarbageCollector::NullInstance)
		{
			void* offset = OffsetMemory(memoryOffset, sizeof(void*));
			memset(offset, 0, sizeof(void*));
			return;
		}

		ObjectInstance* oldValue = GetElement(index);
		if (oldValue != nullptr)
			GarbageCollector::DestroyInstance(oldValue);

		instance->IncrementReference();
		WriteMemory(memoryOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw runtime_error("cannot write null value to ValueType field");

		WriteMemory(memoryOffset, type->MemoryBytesSize, instance->Ptr);
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
