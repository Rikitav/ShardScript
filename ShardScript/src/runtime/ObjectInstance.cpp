#include <shard/runtime/ObjectInstance.hpp>

#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cstring>

using namespace shard;

ObjectInstance::ObjectInstance()
	: m_shape(nullptr)
	, m_memory(nullptr)
{ }

ObjectInstance::ObjectInstance(const TypeShape* shape, std::byte* memory)
	: m_shape(shape)
	, m_memory(memory)
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

std::uint32_t ObjectInstance::resolve_field_slot(gmt::Ref<FieldSymbol> field)
{
	gmt::Ref<StructSymbol> owner = field->get_parent().as<StructSymbol>();
	if (owner.is_null())
		return 0;

	std::uint32_t ordinal = 0;
	for (gmt::Ref<FieldSymbol> candidate : owner->fields)
	{
		if (candidate.is_null() || candidate->get_return_type().is_null())
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
	return ObjectInstance(info.fieldShape, m_memory + info.offset);
}

ObjectInstance ObjectInstance::get_field(gmt::Ref<FieldSymbol> field) const
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

	std::memcpy(fieldAddress, value.get_memory(), info.fieldShape->get_size());
}

void ObjectInstance::set_field(gmt::Ref<FieldSymbol> field, const ObjectInstance& value)
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

std::byte* ObjectInstance::as_pointer() const
{
	std::byte* value;
	std::memcpy(&value, m_memory, sizeof(std::byte*));
	return value;
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
