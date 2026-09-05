#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/RuntimeException.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <cstdint>

using namespace shard;

const TypeSymbol* ObjectInstance::getInfo() const
{
	return m_info;
}

TypeShape* ObjectInstance::getShape() const
{
	return m_shape;
}

std::byte* ObjectInstance::getMemory() const
{
	return m_rawMemoryPtr;
}

std::int64_t ObjectInstance::getReferencesCounter() const
{
	if (GcHeader* header = getGcHeader(); header != nullptr)
		return header->ReferencesCounter;

	return 0;
}

ObjectInstance ObjectInstance::GetField(std::uint32_t slot)
{
	if (m_shape == nullptr)
		throw shard::runtime_exception(L"Cannot read field from an instance without a type shape");

	if (IsNullInstance())
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"null";
		throw shard::runtime_exception(L"Cannot read field on null instance of type : " + typeName);
	}

	if (slot >= m_shape->Slots.size())
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"null";
		throw shard::runtime_exception(L"Field slot index is out of range on : " + typeName);
	}

	TypeShape* fieldShape = m_shape->GetFieldShape(slot);
	if (fieldShape == nullptr)
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"null";
		throw shard::runtime_exception(L"Cannot read instance from a field without a type shape");
	}

	std::size_t fieldOffset = m_shape->GetOffset(slot);
	if (fieldShape->IsReferenceType())
	{
		std::byte* stored = nullptr;
		ReadMemory(fieldOffset, sizeof(void*), &stored);
		return ObjectInstance(fieldShape->BaseType, fieldShape, stored);
	}

	std::byte* offset = OffsetMemory(fieldOffset, fieldShape->Size);
	return ObjectInstance(fieldShape->BaseType, fieldShape, offset);
}

ObjectInstance ObjectInstance::GetField(const FieldSymbol* field)
{
	return GetField(field->SlotIndex);
}

void ObjectInstance::SetField(std::uint32_t slot, ObjectInstance instance)
{
	if (m_shape == nullptr)
		throw shard::runtime_exception(L"Cannot write field on an instance without a type shape");

	if (IsNullInstance())
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"null";
		throw shard::runtime_exception(L"Cannot write field on null instance of type " + typeName);
	}

	if (slot >= m_shape->Slots.size())
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"?";
		throw shard::runtime_exception(L"Field slot index is out of range on " + typeName);
	}

	TypeShape* fieldShape = m_shape->GetFieldShape(slot);
	if (fieldShape == nullptr)
	{
		std::wstring typeName = m_info != nullptr ? m_info->FullName : L"null";
		throw shard::runtime_exception(L"Cannot write instance to a field without a type shape");
	}

	std::size_t fieldOffset = m_shape->GetOffset(slot);
	if (fieldShape->IsReferenceType())
	{
		ObjectInstance oldValue = GetField(slot);
		if (!oldValue.IsNullInstance())
			oldValue.DecrementReference();

		std::byte* stored = instance.getMemory();
		if (!instance.IsNullInstance())
			instance.IncrementReference();

		WriteMemory(fieldOffset, sizeof(void*), &stored);
	}
	else
	{
		if (instance.IsNullInstance())
			throw shard::runtime_exception(L"cannot write null value to ValueType field");

		const TypeSymbol* instanceType = instance.getInfo();
		const TypeSymbol* fieldBaseType = fieldShape->BaseType;

		// Allow assigning a plain Integer value to an enum-typed field.
		if (fieldBaseType != nullptr && fieldBaseType->Kind == SyntaxKind::EnumDeclaration && instanceType == TYPE_INT)
		{
			WriteMemory(fieldOffset, fieldShape->Size, instance.getMemory());
			return;
		}

		WriteMemory(fieldOffset, fieldShape->Size, instance.getMemory());
	}
}

void ObjectInstance::SetField(const FieldSymbol* field, ObjectInstance instance)
{
	SetField(field->SlotIndex, instance);
}

std::size_t ObjectInstance::GetArrayLength() const
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw shard::runtime_exception(L"Tried to get element from non array instance");

	std::int64_t payloadLength;
	std::memcpy(&payloadLength, getMemory(), sizeof(payloadLength));
	return static_cast<std::size_t>(payloadLength);
}

ObjectInstance ObjectInstance::GetElement(std::size_t index, CallStackFrame* frame)
{
	if (IsNullInstance())
		throw shard::runtime_exception(L"Cannot access array element on null instance");

	if (m_info->Kind != SyntaxKind::ArrayType)
		throw shard::runtime_exception(L"Tried to get element from non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(m_info);
	TypeSymbol* underlayingType = info->UnderlayingType;

	if (index >= GetArrayLength())
		throw shard::runtime_exception(L"Array index is out of range: index=" + std::to_wstring(index) + L" length=" + std::to_wstring(GetArrayLength()));

	if (frame != nullptr)
		underlayingType = frame->ResolveType(underlayingType);

	std::size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + underlayingType->GetInlineSize() * index;

	if (underlayingType->IsReferenceType())
	{
		std::byte* stored = nullptr;
		ReadMemory(memoryOffset, sizeof(void*), &stored);
		return ObjectInstance(underlayingType, nullptr, stored);
	}

	std::byte* offset = OffsetMemory(memoryOffset, underlayingType->GetInlineSize());
	return ObjectInstance(underlayingType, nullptr, offset);
}

void ObjectInstance::SetElement(std::size_t index, ObjectInstance instance, CallStackFrame* frame)
{
	if (IsNullInstance())
		throw shard::runtime_exception(L"Cannot access array element on null instance");

	if (m_info->Kind != SyntaxKind::ArrayType)
		throw shard::runtime_exception(L"Tried to set element in non array instance");

	const ArrayTypeSymbol* info = static_cast<const ArrayTypeSymbol*>(m_info);
	TypeSymbol* type = info->UnderlayingType;

	if (index >= GetArrayLength())
		throw shard::runtime_exception(L"Array index is out of range: index=" + std::to_wstring(index) + L" length=" + std::to_wstring(GetArrayLength()));

	if (frame != nullptr)
		type = frame->ResolveType(type);

	std::size_t memoryOffset = SymbolTable::Primitives::Array->MemoryBytesSize + type->GetInlineSize() * index;

	if (type->IsReferenceType())
	{
		ObjectInstance oldValue = GetElement(index, frame);
		if (!oldValue.IsNullInstance())
			oldValue.DecrementReference();

		std::byte* stored = instance.getMemory();
		if (stored != nullptr)
			instance.IncrementReference();

		WriteMemory(memoryOffset, sizeof(void*), &stored);
	}
	else
	{
		if (instance.IsNullInstance())
			throw shard::runtime_exception(L"cannot write null value to by-value type field");

		WriteMemory(memoryOffset, type->MemoryBytesSize, instance.getMemory());
	}
}

bool ObjectInstance::IsInBounds(std::size_t index)
{
	if (m_info->Kind != SyntaxKind::ArrayType)
		throw shard::runtime_exception(L"Tried to get size of non array instance");

	return index < GetArrayLength();
}

void ObjectInstance::IncrementReference() const
{
	if (GcHeader* header = getGcHeader(); header != nullptr)
		header->ReferencesCounter += 1;
}

void ObjectInstance::DecrementReference() const
{
	if (GcHeader* header = getGcHeader(); header != nullptr)
	{
		if (header->ReferencesCounter > 0)
			header->ReferencesCounter -= 1;
	}
}

bool ObjectInstance::IsNullInstance() const
{
	return m_rawMemoryPtr == nullptr;
}

std::byte* ObjectInstance::OffsetMemory(const std::size_t offset, const std::size_t size) const
{
	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (m_info != nullptr && m_info->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(m_info);
		instanceSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->GetInlineSize() * GetArrayLength();
	}

	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	return reinterpret_cast<std::byte*>(getMemory()) + offset;
}

void ObjectInstance::ReadMemory(const std::size_t offset, const std::size_t size, void* dst) const
{
	if (!dst)
		throw std::invalid_argument("Destination is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (m_info != nullptr && m_info->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(m_info);
		instanceSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->GetInlineSize() * GetArrayLength();
	}

	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	const char* memOffset = reinterpret_cast<const char*>(getMemory()) + offset;
	std::memcpy(dst, memOffset, size);
}

void ObjectInstance::WriteMemory(const std::size_t offset, const std::size_t size, const void* src) const
{
	if (!src)
		throw std::invalid_argument("Source is nullptr");

	if (size == 0)
		throw std::out_of_range("Cannot read 0 bytes");

	std::size_t instanceSize = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	if (m_info != nullptr && m_info->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* arrayInfo = static_cast<const ArrayTypeSymbol*>(m_info);
		instanceSize = SymbolTable::Primitives::Array->MemoryBytesSize + arrayInfo->UnderlayingType->GetInlineSize() * GetArrayLength();
	}

	if (offset + size > instanceSize)
		throw std::out_of_range("offset (" + std::to_string(offset) + ") + size (" + std::to_string(size) + ") is out of instance's memory range (" + std::to_string(instanceSize) + ").");

	char* memOffset = reinterpret_cast<char*>(getMemory()) + offset;
	std::memcpy(memOffset, src, size);
}

void ObjectInstance::WriteBoolean(const bool& value) const
{
	if (getInfo() != TYPE_BOOL)
		throw shard::runtime_exception(L"Cannot interpret instance as Boolean");

	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, &value);
}

void ObjectInstance::WriteInteger(const std::int64_t& value) const
{
	if (getInfo() != TYPE_INT && getInfo()->Kind != SyntaxKind::EnumDeclaration)
		throw shard::runtime_exception(L"Cannot interpret instance as Integer");

	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, &value);
}

void ObjectInstance::WriteDouble(const double& value) const
{
	if (getInfo() != TYPE_DOUBLE)
		throw shard::runtime_exception(L"Cannot interpret instance as Double");

	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, &value);
}

void ObjectInstance::WriteCharacter(const wchar_t& value) const
{
	if (getInfo() != TYPE_CHAR)
		throw shard::runtime_exception(L"Cannot interpret instance as Character");

	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, &value);
}

void ObjectInstance::WriteByte(const std::uint8_t& value) const
{
	if (getInfo() != TYPE_BYTE)
		throw shard::runtime_exception(L"Cannot interpret instance as Byte");

	std::size_t size = m_shape != nullptr ? m_shape->Size : (m_info != nullptr ? m_info->MemoryBytesSize : 0);
	WriteMemory(0, size, &value);
}

bool& ObjectInstance::AsBoolean() const
{
	if (getInfo() != TYPE_BOOL)
		throw shard::runtime_exception(L"Cannot interpret instance as Boolean");

	return *reinterpret_cast<bool*>(getMemory());
}

std::int64_t& ObjectInstance::AsInteger() const
{
	if (getInfo() != TYPE_INT && getInfo()->Kind != SyntaxKind::EnumDeclaration)
		throw shard::runtime_exception(L"Cannot interpret instance as Integer");

	return *reinterpret_cast<std::int64_t*>(getMemory());
}

double& ObjectInstance::AsDouble() const
{
	if (getInfo() != TYPE_DOUBLE)
		throw shard::runtime_exception(L"Cannot interpret instance as Double");

	return *reinterpret_cast<double*>(getMemory());
}

wchar_t& ObjectInstance::AsCharacter() const
{
	if (getInfo() != TYPE_CHAR)
		throw shard::runtime_exception(L"Cannot interpret instance as Character");

	return *reinterpret_cast<wchar_t*>(getMemory());
}

std::uint8_t& ObjectInstance::AsByte() const
{
	if (getInfo() != TYPE_BYTE)
		throw shard::runtime_exception(L"Cannot interpret instance as Byte");

	return *reinterpret_cast<std::uint8_t*>(getMemory());
}

std::int64_t& ObjectInstance::AsStringLength() const
{
	if (getInfo() != TYPE_STRING)
		throw shard::runtime_exception(L"Cannot interpret instance as String");

	return *reinterpret_cast<std::int64_t*>(getMemory());
}

const wchar_t* ObjectInstance::AsString() const
{
	if (getInfo() != TYPE_STRING)
		throw shard::runtime_exception(L"Cannot interpret instance as String");

	return *reinterpret_cast<const wchar_t**>(OffsetMemory(sizeof(std::int64_t), sizeof(wchar_t*)));
}

void* ObjectInstance::AsNint() const
{
	if (getInfo() != TYPE_NINT)
		throw shard::runtime_exception(L"Cannot interpret instance as Nint");

	return *reinterpret_cast<void**>(getMemory());
}
