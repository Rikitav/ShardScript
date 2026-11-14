#include <shard/runtime/ObjectInstance.hpp>

#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Allocator.hpp>

#include <stdexcept>

using namespace shard;

ObjectInstance::ObjectInstance()
	: m_shape(nullptr)
	, m_memory(nullptr)
{ }

ObjectInstance::ObjectInstance(const TypeShape* shape, std::byte* memory, GarbageCollector* collector)
	: m_shape(shape)
	, m_memory(memory)
	, m_collector(collector)
{ }

ObjectInstance ObjectInstance::null()
{
	return ObjectInstance(nullptr, nullptr);
}

bool ObjectInstance::is_null() const
{
	return m_memory == nullptr;
}

const TypeShape* ObjectInstance::get_shape() const
{
	return m_shape;
}

std::byte* ObjectInstance::get_memory() const
{
	return m_memory;
}

std::uint32_t ObjectInstance::resolve_field_slot(FieldSymbol* field)
{
	StructSymbol* owner = gmt::as<StructSymbol>(field->get_parent());
	if (owner == nullptr)
		return 0;

	std::uint32_t ordinal = 0;
	for (FieldSymbol* candidate : owner->fields)
	{
		if ((candidate == nullptr) || (candidate->get_return_type() == nullptr))
			continue;

		if (candidate->get_linking() == LINK_STATIC)
			continue;

		if (candidate == field)
			return ordinal;

		++ordinal;
	}

	return 0;
}

ObjectInstance ObjectInstance::get_field(std::uint32_t slot) const
{
	const TypeShape::SlotInfo& info = m_shape->get_slot(slot);
	std::byte* fieldPointer = nullptr;
	std::memcpy(&fieldPointer, m_memory + info.offset, sizeof(std::byte*));
	return ObjectInstance(info.fieldShape, fieldPointer, m_collector);
}

ObjectInstance ObjectInstance::get_field(FieldSymbol* field) const
{
	return get_field(resolve_field_slot(field));
}

void ObjectInstance::set_field(std::uint32_t slot, const ObjectInstance& value)
{
	const TypeShape::SlotInfo& info = m_shape->get_slot(slot);
	std::byte* fieldAddress = m_memory + info.offset;

	if (info.fieldShape->is_reference())
	{
		std::byte* pointer = value.get_memory();
		std::memcpy(fieldAddress, &pointer, sizeof(std::byte*));
		return;
	}

	if (value == nullptr)
	{
		std::byte* pointer = nullptr;
		std::memcpy(fieldAddress, &pointer, sizeof(std::byte*));
		return;
	}

	std::byte* fieldPointer = nullptr;
	std::memcpy(&fieldPointer, fieldAddress, sizeof(std::byte*));
	if (fieldPointer == nullptr)
	{
		if (m_collector == nullptr)
			throw std::runtime_error("cannot allocate a value cell without a garbage collector");

		ObjectInstance cell = m_collector->allocate_instance(info.fieldShape);
		fieldPointer = cell.get_memory();
		std::memcpy(fieldAddress, &fieldPointer, sizeof(std::byte*));
	}

	std::memcpy(fieldPointer, value.get_memory(), info.fieldShape->get_size());
}

void ObjectInstance::set_field(FieldSymbol* field, const ObjectInstance& value)
{
	set_field(resolve_field_slot(field), value);
}

bool ObjectInstance::as_boolean() const
{
	bool value;
	std::memcpy(&value, m_memory, sizeof(bool));
	return value;
}

std::int64_t ObjectInstance::as_integer() const
{
	if (m_shape != nullptr && m_shape->get_size() == 1)
		return static_cast<std::int64_t>(*reinterpret_cast<const std::uint8_t*>(m_memory));

	std::int64_t value;
	std::memcpy(&value, m_memory, sizeof(std::int64_t));
	return value;
}

double ObjectInstance::as_double() const
{
	double value;
	std::memcpy(&value, m_memory, sizeof(double));
	return value;
}

wchar_t ObjectInstance::as_char() const
{
	wchar_t value;
	std::memcpy(&value, m_memory, sizeof(wchar_t));
	return value;
}

std::uintptr_t ObjectInstance::as_nint() const
{
	std::uintptr_t value;
	std::memcpy(&value, m_memory, sizeof(std::uintptr_t));
	return value;
}

std::byte* ObjectInstance::as_pointer() const
{
	if (m_shape != nullptr && m_shape->is_reference())
		return m_memory;

	std::byte* value;
	std::memcpy(&value, m_memory, sizeof(std::byte*));
	return value;
}

std::wstring_view ObjectInstance::as_string() const
{
	std::int64_t length{};
	std::memcpy(&length, m_memory, sizeof(std::int64_t));

	if (length <= 0)
		return std::wstring_view();

	const wchar_t* data = reinterpret_cast<const wchar_t*>(m_memory + sizeof(std::int64_t));
	return std::wstring_view(data, length);
}

void ObjectInstance::write_boolean(bool value)
{
	std::memcpy(m_memory, &value, sizeof(bool));
}

void ObjectInstance::write_integer(std::int64_t value)
{
	std::memcpy(m_memory, &value, sizeof(std::int64_t));
}

void ObjectInstance::write_double(double value)
{
	std::memcpy(m_memory, &value, sizeof(double));
}

void ObjectInstance::write_char(wchar_t value)
{
	std::memcpy(m_memory, &value, sizeof(wchar_t));
}

void ObjectInstance::write_pointer(std::byte* value)
{
	std::memcpy(m_memory, &value, sizeof(std::byte*));
}
