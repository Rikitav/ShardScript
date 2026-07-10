#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/SemanticModel.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <cstdint>

using namespace shard;

static TypeSymbol* ResolveRuntimeType(TypeSymbol* type, CallStackFrame* frame, TypeShape* instanceShape)
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

        if (instanceShape != nullptr && instanceShape->BaseType->TypeParameters.size() > 0)
        {
            TypeParameterSymbol* typeParameter = static_cast<TypeParameterSymbol*>(type);
            for (std::size_t i = 0; i < instanceShape->BaseType->TypeParameters.size(); ++i)
            {
                if (instanceShape->BaseType->TypeParameters[i] == typeParameter)
                    return i < instanceShape->GenericArguments.size() ? instanceShape->GenericArguments[i] : type;
            }
        }

        return type;
    }

    if (type->Kind == SyntaxKind::ArrayType)
    {
        ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
        TypeSymbol* resolvedUnderlying = ResolveRuntimeType(arrayType->UnderlayingType, frame, instanceShape);
        if (resolvedUnderlying == nullptr || resolvedUnderlying == arrayType->UnderlayingType)
            return type;

        ArrayTypeSymbol* resolvedArray = new ArrayTypeSymbol(resolvedUnderlying);
        resolvedArray->Length = arrayType->Length;
        resolvedArray->LayoutingState = arrayType->LayoutingState;
        return resolvedArray;
    }

    return type;
}

static bool RuntimeTypeEquals(TypeSymbol* expected, TypeSymbol* actual, CallStackFrame* frame, TypeShape* instanceShape)
{
    if (expected == actual)
        return true;
    if (expected == nullptr || actual == nullptr)
        return false;

    if (expected->Kind == SyntaxKind::TypeParameter)
    {
        TypeSymbol* resolved = ResolveRuntimeType(expected, frame, instanceShape);
        if (resolved == expected)
            return SemanticModel::AreTypesEqual(expected, actual);

        return RuntimeTypeEquals(resolved, actual, frame, instanceShape);
    }

    if (expected->Kind == SyntaxKind::ArrayType)
    {
        if (actual->Kind != SyntaxKind::ArrayType)
            return false;

        ArrayTypeSymbol* expectedArray = static_cast<ArrayTypeSymbol*>(expected);
        ArrayTypeSymbol* actualArray = static_cast<ArrayTypeSymbol*>(actual);

        return RuntimeTypeEquals(expectedArray->UnderlayingType, actualArray->UnderlayingType, frame, instanceShape);
    }

    return SemanticModel::AreTypesEqual(expected, actual);
}

const TypeSymbol* ObjectInstance::getInfo() const
{
	return m_info;
}

TypeShape* ObjectInstance::getShape() const
{
	return m_shape;
}

bool shard::ObjectInstance::getIsTransient() const
{
	return m_isTransient;
}

void* ObjectInstance::getMemory() const
{
	return m_rawMemoryPtr;
}

std::int64_t ObjectInstance::getReferencesCounter() const
{
	return m_eeferencesCounter;
}

ObjectInstance* ObjectInstance::GetField(std::uint32_t slot)
{
	if (m_shape == nullptr)
		throw std::runtime_error("Cannot read field from an instance without a type shape");

	if (slot >= m_shape->Slots.size())
		throw std::runtime_error("Field slot index is out of range");

	TypeShape* fieldShape = m_shape->GetFieldShape(slot);
	std::size_t fieldOffset = m_shape->GetOffset(slot);

	if (fieldShape == nullptr || fieldShape->IsReferenceType())
	{
		void* offset = OffsetMemory(fieldOffset, sizeof(ObjectInstance*));
		ObjectInstance* valuePtr = *static_cast<ObjectInstance**>(offset);
		return valuePtr == nullptr ? GarbageCollector::NullInstance : valuePtr;
	}
	else
	{
		void* offset = OffsetMemory(fieldOffset, fieldShape->Size);
		ObjectInstance* instance = new ObjectInstance(fieldShape->BaseType, fieldShape, offset, true);
		return instance;
	}
}

void ObjectInstance::SetField(std::uint32_t slot, ObjectInstance* instance)
{

	if (m_shape == nullptr)
		throw std::runtime_error("Cannot write field on an instance without a type shape");

	if (slot >= m_shape->Slots.size())
		throw std::runtime_error("Field slot index is out of range");

	if (instance == nullptr)
		throw std::runtime_error("Tried to set nullptr ObjectInstance as field value.");

	TypeShape* fieldShape = m_shape->GetFieldShape(slot);
	std::size_t fieldOffset = m_shape->GetOffset(slot);

	if (fieldShape == nullptr || fieldShape->IsReferenceType())
	{
		ObjectInstance* oldValue = GetField(slot);
		if (oldValue != nullptr && oldValue != GarbageCollector::NullInstance)
			oldValue->DecrementReference();

		if (instance != GarbageCollector::NullInstance)
			instance->IncrementReference();

		WriteMemory(fieldOffset, sizeof(ObjectInstance*), &instance);
	}
	else
	{
		if (instance == GarbageCollector::NullInstance)
			throw std::runtime_error("cannot write null value to ValueType field");

		TypeShape* instanceShape = instance->getShape();
		const TypeSymbol* instanceType = instance->getInfo();
		const TypeSymbol* fieldBaseType = fieldShape->BaseType;

		// Allow assigning a plain Integer value to an enum-typed field.
		if (fieldBaseType != nullptr && fieldBaseType->Kind == SyntaxKind::EnumDeclaration && instanceType == TYPE_INT)
		{
			WriteMemory(fieldOffset, fieldShape->Size, instance->getMemory());
			return;
		}

		if (instanceShape != nullptr && instanceShape != fieldShape)
		{
			std::string msg = "Tried to set incompatible ObjectInstance type as field value. Field base: ";
			msg += fieldShape->BaseType != nullptr ? std::string(fieldShape->BaseType->Name.begin(), fieldShape->BaseType->Name.end()) : "null";
			msg += ", instance base: ";
			msg += instanceShape->BaseType != nullptr ? std::string(instanceShape->BaseType->Name.begin(), instanceShape->BaseType->Name.end()) : "null";
			throw std::runtime_error(msg);
		}

		if (instanceShape == nullptr && instanceType != fieldShape->BaseType)
		{
			std::string msg = "Tried to set incompatible ObjectInstance type as field value. Field base: ";
			msg += fieldShape->BaseType != nullptr ? std::string(fieldShape->BaseType->Name.begin(), fieldShape->BaseType->Name.end()) : "null";
			msg += ", instance base: null";
			throw std::runtime_error(msg);
		}

		WriteMemory(fieldOffset, fieldShape->Size, instance->getMemory());
	}
}

std::size_t ObjectInstance::GetArrayLength() const
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get element from non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(m_info);
	return info->Length;
}

ObjectInstance* ObjectInstance::GetElement(std::size_t index, CallStackFrame* frame)
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get element from non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(m_info);
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
		ObjectInstance* instance = new ObjectInstance(type, nullptr, offset, true);
		return instance;
	}
}

void ObjectInstance::SetElement(std::size_t index, ObjectInstance* instance, CallStackFrame* frame)
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to set element in non array instance");

	if (instance == nullptr)
		throw std::runtime_error("got nullptr instance in SetElement");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(m_info);
	TypeSymbol* type = info->UnderlayingType;

	if (frame != nullptr)
		type = frame->ResolveType(type);

	std::size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;
	if (type->IsReferenceType())
	{
		ObjectInstance* oldValue = GetElement(index, frame);
		if (oldValue != nullptr && oldValue != GarbageCollector::NullInstance)
			oldValue->DecrementReference();

		if (instance != GarbageCollector::NullInstance)
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
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get size of non array instance");

	return index >= 0 && index < GetArrayLength();
}

ArgumentsSpan ObjectInstance::ArrayAsSpan()
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw std::runtime_error("Tried to get args span of non array instance");

	size_t length = GetArrayLength();
	if (length == 0)
		return ArgumentsSpan();

	ObjectInstance* first = GetElement(0);
	return ArgumentsSpan{ &first, length };
}

void ObjectInstance::IncrementReference()
{
	if (m_eeferencesCounter == (std::size_t)(-1))
		return;

	m_eeferencesCounter += 1;
}

void ObjectInstance::DecrementReference()
{
	if (m_eeferencesCounter == 0)
		return;

	m_eeferencesCounter -= 1;
}

void* ObjectInstance::OffsetMemory(const std::size_t offset, const std::size_t size) const
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	return static_cast<char*>(getMemory()) + offset;
}

void ObjectInstance::ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	const char* memOffset = static_cast<char*>(getMemory()) + offset;
	std::memcpy(dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	char* memOffset = static_cast<char*>(getMemory()) + offset;
	std::memcpy(memOffset, src, size);
}

void ObjectInstance::WriteBoolean(const bool& value) const
{
	const void* ptr = &value;
	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, ptr);
}

void ObjectInstance::WriteInteger(const std::int64_t& value) const
{
	const void* ptr = &value;
	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, ptr);
}

void ObjectInstance::WriteDouble(const double& value) const
{
	const void* ptr = &value;
	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, ptr);
}

void ObjectInstance::WriteCharacter(const wchar_t& value) const
{
	const void* ptr = &value;
	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, ptr);
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

std::int64_t& ObjectInstance::AsStringLength() const
{
	if (getInfo() != TYPE_STRING)
		throw std::runtime_error("Cannot interpret instance as String");

	return *reinterpret_cast<std::int64_t*>(getMemory());
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
