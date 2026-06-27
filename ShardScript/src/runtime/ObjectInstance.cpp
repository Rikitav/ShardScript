#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <cstdint>

using namespace shard;

static TypeSymbol* ResolveRuntimeType(TypeSymbol* type, CallStackFrame* frame, GenericTypeSymbol* genericInfo)
{
    if (type == nullptr)
        return nullptr;

    if (type->Kind == SyntaxKind::TypeParameter)
    {
        if (frame != nullptr)
        {
            TypeSymbol* resolved = frame->ResolveType(type);
            if (resolved != type)
                return resolved;
        }

        if (genericInfo != nullptr)
            return genericInfo->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(type));

        return type;
    }

    if (type->Kind == SyntaxKind::ArrayType)
    {
        ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
        TypeSymbol* resolvedUnderlying = ResolveRuntimeType(arrayType->UnderlayingType, frame, genericInfo);
        if (resolvedUnderlying == nullptr || resolvedUnderlying == arrayType->UnderlayingType)
            return type;

        ArrayTypeSymbol* resolvedArray = new ArrayTypeSymbol(resolvedUnderlying);
        resolvedArray->Length = arrayType->Length;
        resolvedArray->State = arrayType->State;
        return resolvedArray;
    }

    return type;
}

static bool RuntimeTypeEquals(TypeSymbol* expected, TypeSymbol* actual, CallStackFrame* frame, GenericTypeSymbol* genericInfo)
{
    if (expected == actual)
        return true;
    if (expected == nullptr || actual == nullptr)
        return false;

    if (expected->Kind == SyntaxKind::TypeParameter)
    {
        TypeSymbol* resolved = ResolveRuntimeType(expected, frame, genericInfo);
        if (resolved == expected)
            return TypeSymbol::Equals(expected, actual);

        return RuntimeTypeEquals(resolved, actual, frame, genericInfo);
    }

    if (expected->Kind == SyntaxKind::ArrayType)
    {
        if (actual->Kind != SyntaxKind::ArrayType)
            return false;

        ArrayTypeSymbol* expectedArray = static_cast<ArrayTypeSymbol*>(expected);
        ArrayTypeSymbol* actualArray = static_cast<ArrayTypeSymbol*>(actual);

        return RuntimeTypeEquals(expectedArray->UnderlayingType, actualArray->UnderlayingType, frame, genericInfo);
    }

    if (expected->Kind == SyntaxKind::GenericType)
    {
        if (actual->Kind != SyntaxKind::GenericType)
            return false;

        GenericTypeSymbol* expectedGeneric = static_cast<GenericTypeSymbol*>(expected);
        GenericTypeSymbol* actualGeneric = static_cast<GenericTypeSymbol*>(actual);

        if (!RuntimeTypeEquals(expectedGeneric->UnderlayingType, actualGeneric->UnderlayingType, frame, genericInfo))
            return false;

        for (TypeParameterSymbol* typeParam : expectedGeneric->UnderlayingType->TypeParameters)
        {
            TypeSymbol* expectedArg = expectedGeneric->SubstituteTypeParameters(typeParam);
            TypeSymbol* actualArg = actualGeneric->SubstituteTypeParameters(typeParam);

            if (expectedArg == nullptr)
                expectedArg = typeParam;
            
            if (actualArg == nullptr)
                actualArg = typeParam;

            if (!RuntimeTypeEquals(expectedArg, actualArg, frame, genericInfo))
                return false;
        }

        return true;
    }

    return TypeSymbol::Equals(expected, actual);
}

static GenericTypeSymbol* GetGenericInfo(ObjectInstance* instance)
{
    const TypeSymbol* info = instance->getInfo();
    if (info != nullptr && info->Kind == SyntaxKind::GenericType)
        return const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(info));

    return nullptr;
}

const TypeSymbol* ObjectInstance::getInfo() const
{
	return Info;
}

bool shard::ObjectInstance::getIsTransient() const
{
	return IsTransient;
}

void* ObjectInstance::getMemory() const
{
	return Memory;
}

std::int64_t ObjectInstance::getReferencesCounter() const
{
	return ReferencesCounter;
}

ObjectInstance* ObjectInstance::GetField(FieldSymbol* field, CallStackFrame* frame)
{
	GenericTypeSymbol* genericInfo = GetGenericInfo(this);
	TypeSymbol* fieldType = ResolveRuntimeType(field->ReturnType, frame, genericInfo);

	if (fieldType->IsReferenceType())
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*));
		ObjectInstance* valuePtr = *static_cast<ObjectInstance**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : valuePtr;
	}
	else
	{
		void* offset = OffsetMemory(field->MemoryBytesOffset, fieldType->GetInlineSize());
		ObjectInstance* instance = new ObjectInstance(fieldType, offset, true);
		return instance;
	}
}

void ObjectInstance::SetField(FieldSymbol* field, ObjectInstance* instance, CallStackFrame* frame)
{
	if (instance == nullptr)
		throw std::runtime_error("got nullptr instance");

	GenericTypeSymbol* genericInfo = GetGenericInfo(this);
	TypeSymbol* fieldType = field->ReturnType;
	if (!RuntimeTypeEquals(fieldType, const_cast<TypeSymbol*>(instance->getInfo()), frame, genericInfo))
		throw std::runtime_error("incompatible field type");

	fieldType = ResolveRuntimeType(fieldType, frame, genericInfo);

	if (fieldType->IsReferenceType())
	{
		ObjectInstance* oldValue = GetField(field, frame);
		if (oldValue != nullptr)
		{
			oldValue->DecrementReference();
		}
		
		instance->IncrementReference();
		WriteMemory(field->MemoryBytesOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw std::runtime_error("cannot write null value to ValueType field");
		
		WriteMemory(field->MemoryBytesOffset, fieldType->GetInlineSize(), instance->getMemory());
	}
}

ObjectInstance* ObjectInstance::GetElement(std::size_t index, CallStackFrame* frame)
{
	if (Info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get element from non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;

	if (frame != nullptr)
		type = frame->ResolveType(type);

	std::size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;
	if (type->IsReferenceType())
	{
		void* offset = OffsetMemory(memoryOffset, sizeof(ObjectInstance*));
		ObjectInstance* valuePtr = *static_cast<ObjectInstance**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : valuePtr;
	}
	else
	{
		void* offset = OffsetMemory(memoryOffset, type->MemoryBytesSize);
		ObjectInstance* instance = new ObjectInstance(type, offset, true);
		return instance;
	}
}

void ObjectInstance::SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame)
{
	if (Info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to set element in non array instance");

	if (instance == nullptr)
		throw std::runtime_error("got nullptr instance in SetElement");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(Info);
	TypeSymbol* type = info->UnderlayingType;

	if (frame != nullptr)
		type = frame->ResolveType(type);

	std::size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;
	if (type->IsReferenceType())
	{
		ObjectInstance* oldValue = GetElement(index, frame);
		oldValue->DecrementReference();

		instance->IncrementReference();
		WriteMemory(memoryOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw std::runtime_error("cannot write null value to ValueType field");

		WriteMemory(memoryOffset, type->MemoryBytesSize, instance->getMemory());
	}
}

bool ObjectInstance::IsInBounds(std::size_t index)
{
	if (Info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get size of non array instance");

	return index >= 0 && index < GetArrayLength();
}

void ObjectInstance::IncrementReference()
{
	if (ReferencesCounter == (std::size_t)(-1))
		return;

	ReferencesCounter += 1;
}

void ObjectInstance::DecrementReference()
{
	if (ReferencesCounter == 0)
		return;

	ReferencesCounter -= 1;
}

void* ObjectInstance::OffsetMemory(const std::size_t offset, const std::size_t size) const
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > GetMemorySize())
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(GetMemorySize()) + ").");

	return static_cast<char*>(getMemory()) + offset;
}

void ObjectInstance::ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > GetMemorySize())
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(GetMemorySize()) + ").");

	const char* memOffset = static_cast<char*>(getMemory()) + offset;
	std::memcpy(dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	if (offset + size > GetMemorySize())
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(GetMemorySize()) + ").");

	char* memOffset = static_cast<char*>(getMemory()) + offset;
	std::memcpy(memOffset, src, size);
}

void ObjectInstance::WriteBoolean(const bool& value) const
{
	const void* ptr = &value;
	WriteMemory(0, GetMemorySize(), ptr);
}

void ObjectInstance::WriteInteger(const std::int64_t& value) const
{
	const void* ptr = &value;
	WriteMemory(0, GetMemorySize(), ptr);
}

void ObjectInstance::WriteDouble(const double& value) const
{
	const void* ptr = &value;
	WriteMemory(0, GetMemorySize(), ptr);
}

void ObjectInstance::WriteCharacter(const wchar_t& value) const
{
	const void* ptr = &value;
	WriteMemory(0, GetMemorySize(), ptr);
}

void ObjectInstance::WriteString(const wchar_t* value) const
{
	std::size_t size = wcslen(value);
	WriteString(value, size);
}

void ObjectInstance::WriteString(const wchar_t* value, std::size_t size) const
{
	WriteMemory(0, sizeof(std::int64_t), &size);
	WriteMemory(sizeof(std::int64_t), sizeof(wchar_t) * size, value);
}

void ObjectInstance::WriteString(const std::wstring& value) const
{
	WriteString(value.data(), value.size());
}

bool& ObjectInstance::AsBoolean() const
{
	if (getInfo() != TYPE_BOOL)
		throw std::runtime_error("Cannot interpret instance as Boolean");

	return *reinterpret_cast<bool*>(getMemory());
}

std::int64_t& ObjectInstance::AsInteger() const
{
	if (getInfo() != TYPE_INT && getInfo()->Kind != SyntaxKind::EnumDeclaration)
		throw std::runtime_error("Cannot interpret instance as Integer");

	return *reinterpret_cast<std::int64_t*>(getMemory());
}

double& ObjectInstance::AsDouble() const
{
	if (getInfo() != TYPE_DOUBLE)
		throw std::runtime_error("Cannot interpret instance as Double");

	return *reinterpret_cast<double*>(getMemory());
}

wchar_t& ObjectInstance::AsCharacter() const
{
	if (getInfo() != TYPE_CHAR)
		throw std::runtime_error("Cannot interpret instance as Character");

	return *reinterpret_cast<wchar_t*>(getMemory());
}

const wchar_t* ObjectInstance::AsString() const
{
	if (getInfo() != TYPE_STRING)
		throw std::runtime_error("Cannot interpret instance as String");

	return *reinterpret_cast<const wchar_t**>(OffsetMemory(sizeof(std::int64_t), sizeof(wchar_t*)));
}

void* ObjectInstance::AsNint() const
{
	if (getInfo() != TYPE_NINT)
		throw std::runtime_error("Cannot interpret instance as Nint");

	return *reinterpret_cast<void**>(getMemory());
}
