#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/AbstractInterpreter.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <cstring>
#include <stdexcept>
#include <string>

using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

ObjectInstance* ObjectInstance::FromValue(bool value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Boolean);
	instance->WriteBoolean(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(long value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	instance->WriteInteger(value);
	return instance;
}

ObjectInstance* shard::runtime::ObjectInstance::FromValue(double value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Double);
	instance->WriteDouble(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(wchar_t value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Char);
	instance->WriteCharacter(value);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(const wchar_t* value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
	std::wstring copy = std::wstring(value);
	instance->WriteString(copy);
	return instance;
}

ObjectInstance* ObjectInstance::FromValue(const std::wstring& value)
{
	ObjectInstance* instance = GarbageCollector::AllocateInstance(SymbolTable::Primitives::String);
	instance->WriteString(value);
	return instance;
}

ObjectInstance* ObjectInstance::GetField(FieldSymbol* field)
{
	TypeSymbol* fieldType = field->ReturnType;
	if (fieldType->Kind == SyntaxKind::TypeParameter)
	{
		/*
		CallStackFrame* currentFrame = AbstractInterpreter::CurrentFrame();
		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(currentFrame->WithinType));
		*/

		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(Info));
		fieldType = genericInfo->SubstituteTypeParameters(fieldType);
	}

	if (fieldType->IsReferenceType)
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*));
		void* valuePtr = *static_cast<void**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : static_cast<ObjectInstance*>(valuePtr);
	}
	else
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, fieldType->MemoryBytesSize);
		ObjectInstance* instance = GarbageCollector::CopyInstance(fieldType, offset);
		instance->IsFieldInstance = true;
		return instance;
	}
}

void ObjectInstance::SetField(FieldSymbol* field, ObjectInstance* instance)
{
	if (instance == nullptr)
		throw std::runtime_error("got nullptr instance");

	TypeSymbol* fieldType = field->ReturnType;
	if (fieldType->Kind == SyntaxKind::TypeParameter)
	{
		/*
		CallStackFrame* currentFrame = AbstractInterpreter::CurrentFrame();
		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(currentFrame->WithinType));
		*/
		
		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(Info));
		fieldType = genericInfo->SubstituteTypeParameters(fieldType);
	}

	if (fieldType->IsReferenceType)
	{
		if (instance == GarbageCollector::NullInstance)
		{
			void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(void*));
			memset(offset, 0, sizeof(void*));
			return;
		}

		ObjectInstance* oldValue = GetField(field);
		if (oldValue != nullptr)
			GarbageCollector::CollectInstance(oldValue);
		
		instance->IncrementReference();
		WriteMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw std::runtime_error("cannot write null value to ValueType field");

		WriteMemory(field->MemoryBytesOffset, fieldType->MemoryBytesSize, instance->Ptr);
	}
}

ObjectInstance* ObjectInstance::GetElement(size_t index)
{
	if (Info->Kind != shard::syntax::SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get element from non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;

	if (type->Kind == SyntaxKind::GenericType)
	{
		CallStackFrame* currentFrame = AbstractInterpreter::CurrentFrame();
		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(currentFrame->WithinType));
		type = genericInfo->SubstituteTypeParameters(type);
	}

	size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;
	if (type->IsReferenceType)
	{
		void* offset = OffsetMemory(memoryOffset, sizeof(ObjectInstance*));
		void* valuePtr = *static_cast<void**>(offset);

		if (valuePtr == nullptr)
			throw std::runtime_error("got nullptr in GetElement");

		return static_cast<ObjectInstance*>(valuePtr);
	}
	else
	{
		void* offset = OffsetMemory(memoryOffset, type->MemoryBytesSize);
		ObjectInstance* instance = GarbageCollector::CopyInstance(type, offset);
		instance->IsFieldInstance = true;
		return instance;
	}
}

void ObjectInstance::SetElement(size_t index, ObjectInstance* instance)
{
	if (Info->Kind != shard::syntax::SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to set element in non array instance");

	if (instance == nullptr)
		throw std::runtime_error("got nullptr instance in SetElement");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;

	if (type->Kind == SyntaxKind::GenericType)
	{
		CallStackFrame* currentFrame = AbstractInterpreter::CurrentFrame();
		GenericTypeSymbol* genericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(currentFrame->WithinType));
		type = genericInfo->SubstituteTypeParameters(type);
	}

	size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;
	if (type->IsReferenceType)
	{
		ObjectInstance* oldValue = GetElement(index);
		GarbageCollector::CollectInstance(oldValue);

		instance->IncrementReference();
		WriteMemory(memoryOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw std::runtime_error("cannot write null value to ValueType field");

		WriteMemory(memoryOffset, type->MemoryBytesSize, instance->Ptr);
	}
}

bool ObjectInstance::IsInBounds(size_t index)
{
	if (Info->Kind != shard::syntax::SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get size of non array instance");

	const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(Info);
	return index >= 0 && index < array->Size;
}

void ObjectInstance::IncrementReference()
{
	if (ReferencesCounter == (size_t)(-1))
		return;

	ReferencesCounter += 1;
}

void ObjectInstance::DecrementReference()
{
	if (ReferencesCounter == 0)
		return;

	ReferencesCounter -= 1;
}

void* ObjectInstance::OffsetMemory(const size_t offset, const size_t size) const
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(Info->MemoryBytesSize) + ").");

	return static_cast<char*>(Ptr) + offset;
}

void ObjectInstance::ReadMemory(const size_t offset, const size_t size, void* dst) const
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(Info->MemoryBytesSize) + ").");

	const char* memOffset = static_cast<char*>(Ptr) + offset;
	memcpy(dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const size_t offset, const size_t size, const void* src) const
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > Info->MemoryBytesSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(Info->MemoryBytesSize) + ").");

	char* memOffset = static_cast<char*>(Ptr) + offset;
	memcpy(memOffset, src, size);
}

void ObjectInstance::WriteBoolean(const bool& value)
{
	const void* ptr = &value;
	WriteMemory(0, Info->MemoryBytesSize, ptr);
}

void ObjectInstance::WriteInteger(const long& value)
{
	const void* ptr = &value;
	WriteMemory(0, Info->MemoryBytesSize, ptr);
}

void ObjectInstance::WriteDouble(const double& value)
{
	const void* ptr = &value;
	WriteMemory(0, Info->MemoryBytesSize, ptr);
}

void ObjectInstance::WriteCharacter(const wchar_t& value)
{
	const void* ptr = &value;
	WriteMemory(0, Info->MemoryBytesSize, ptr);
}

void ObjectInstance::WriteString(const std::wstring& value)
{
	const void* ptr = new std::wstring(value);
	WriteMemory(0, Info->MemoryBytesSize, ptr);
}

bool ObjectInstance::AsBoolean()
{
	return *reinterpret_cast<bool*>(Ptr);
}

long ObjectInstance::AsInteger()
{
	return *reinterpret_cast<long*>(Ptr);
}

double shard::runtime::ObjectInstance::AsDouble()
{
	return *reinterpret_cast<double*>(Ptr);
}

wchar_t ObjectInstance::AsCharacter()
{
	return *reinterpret_cast<wchar_t*>(Ptr);
}

std::wstring& ObjectInstance::AsString()
{
	return *reinterpret_cast<std::wstring*>(Ptr);
}
