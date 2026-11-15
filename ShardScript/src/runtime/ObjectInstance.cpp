#include <shard/runtime/ObjectInstance.hpp>

#include <shard/pragmatic/TypeShape.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Allocator.hpp>

#include <initializer_list>
#include <stdexcept>
#include <string>

using namespace shard;

namespace
{
	using ValueKind = TypeShape::ValueKind;

	constexpr std::uint32_t kind_bit(ValueKind kind)
	{
		return 1u << static_cast<unsigned>(kind);
	}

	constexpr std::uint32_t kinds_of(std::initializer_list<ValueKind> kinds)
	{
		std::uint32_t mask = 0;
		for (ValueKind kind : kinds)
			mask |= kind_bit(kind);

		return mask;
	}

	// bytes a value of this kind occupies, 0 when it is not integral
	constexpr std::size_t integral_width(ValueKind kind)
	{
		switch (kind)
		{
			case ValueKind::Byte: return sizeof(std::uint8_t);
			case ValueKind::Char: return sizeof(wchar_t);
			case ValueKind::Integer:
			case ValueKind::NativeInteger:
			case ValueKind::Enum: return sizeof(std::int64_t);
			default: return 0;
		}
	}

	constexpr std::uint32_t IntegralKinds = kinds_of({ ValueKind::Byte, ValueKind::Char, ValueKind::Integer, ValueKind::NativeInteger, ValueKind::Enum });

	std::string narrow(std::wstring_view text)
	{
		std::string result;
		result.reserve(text.size());
		for (const wchar_t symbol : text)
			result.push_back(symbol < 128 ? static_cast<char>(symbol) : '?');

		return result;
	}

	std::string describe(const TypeShape* shape)
	{
		if (shape == nullptr || shape->get_base_type() == nullptr)
			return "?";

		return narrow(shape->get_base_type()->get_name());
	}
}

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
	if (m_shape == nullptr)
		throw std::runtime_error("field access has no type shape");
	if (m_memory == nullptr)
		throw std::runtime_error("field access has no object storage");

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

void ObjectInstance::require_kind(std::uint32_t allowedKinds, const char* accessor) const
{
	if (m_memory == nullptr)
		throw std::logic_error(std::string("type error: ") + accessor + " on a null instance");

	if (m_shape == nullptr)
		throw std::logic_error(std::string("type error: ") + accessor + " without a type shape");

	if ((allowedKinds & kind_bit(m_shape->get_value_kind())) != 0)
		return;

	throw std::logic_error(std::string("type error: ") + accessor + " on '" + describe(m_shape) + "'");
}

void ObjectInstance::require_integral(std::size_t capacity, const char* accessor) const
{
	require_kind(IntegralKinds, accessor);

	const std::size_t width = integral_width(m_shape->get_value_kind());
	if (width > capacity)
		throw std::logic_error(std::string("type error: ") + accessor + " would narrow '" + describe(m_shape) + "' from " + std::to_string(width) + " to " + std::to_string(capacity) + " bytes");
}

bool ObjectInstance::as_boolean() const
{
	require_kind(kind_bit(ValueKind::Boolean), "as_boolean()");

	bool value;
	std::memcpy(&value, m_memory, sizeof(bool));
	return value;
}

std::int64_t ObjectInstance::as_integer() const
{
	require_integral(sizeof(std::int64_t), "as_integer()");

	// lesser integrals hold 1-2 bytes: read exactly those and zero-extend
	const std::size_t width = integral_width(m_shape->get_value_kind());
	if (width < sizeof(std::int64_t))
	{
		std::uint64_t narrowValue = 0;
		std::memcpy(&narrowValue, m_memory, width);
		return static_cast<std::int64_t>(narrowValue);
	}

	std::int64_t value;
	std::memcpy(&value, m_memory, sizeof(std::int64_t));
	return value;
}

double ObjectInstance::as_double() const
{
	require_kind(kind_bit(ValueKind::Double), "as_double()");

	double value;
	std::memcpy(&value, m_memory, sizeof(double));
	return value;
}

wchar_t ObjectInstance::as_char() const
{
	require_integral(sizeof(wchar_t), "as_char()");

	wchar_t value;
	std::memcpy(&value, m_memory, sizeof(wchar_t));
	return value;
}

std::uintptr_t ObjectInstance::as_nint() const
{
	require_integral(sizeof(std::uintptr_t), "as_nint()");

	std::uintptr_t value;
	std::memcpy(&value, m_memory, sizeof(std::uintptr_t));
	return value;
}

std::byte* ObjectInstance::as_pointer() const
{
	// a null reference legitimately yields a null pointer
	if (m_memory == nullptr)
		return nullptr;

	require_kind(kinds_of({ ValueKind::Reference, ValueKind::String, ValueKind::Array, ValueKind::Integer, ValueKind::NativeInteger }), "as_pointer()");

	if (m_shape->is_reference())
		return m_memory;

	std::byte* value;
	std::memcpy(&value, m_memory, sizeof(std::byte*));
	return value;
}

std::wstring_view ObjectInstance::as_string() const
{
	if (m_memory == nullptr)
		return std::wstring_view();

	require_kind(kind_bit(ValueKind::String), "as_string()");

	const std::int64_t length = as_length();
	if (length <= 0)
		return std::wstring_view();

	const wchar_t* data = reinterpret_cast<const wchar_t*>(m_memory + sizeof(std::int64_t));
	return std::wstring_view(data, static_cast<std::size_t>(length));
}

std::int64_t ObjectInstance::as_length() const
{
	require_kind(kinds_of({ ValueKind::String, ValueKind::Array }), "as_length()");

	std::int64_t length;
	std::memcpy(&length, m_memory, sizeof(std::int64_t));
	return length;
}

void ObjectInstance::write_boolean(bool value)
{
	require_kind(kind_bit(ValueKind::Boolean), "write_boolean()");

	std::memcpy(m_memory, &value, sizeof(bool));
}

void ObjectInstance::write_integer(std::int64_t value)
{
	require_kind(kinds_of({ ValueKind::Integer, ValueKind::NativeInteger, ValueKind::Enum }), "write_integer()");

	std::memcpy(m_memory, &value, sizeof(std::int64_t));
}

void ObjectInstance::write_double(double value)
{
	require_kind(kind_bit(ValueKind::Double), "write_double()");

	std::memcpy(m_memory, &value, sizeof(double));
}

void ObjectInstance::write_char(wchar_t value)
{
	require_kind(kind_bit(ValueKind::Char), "write_char()");

	std::memcpy(m_memory, &value, sizeof(wchar_t));
}

void ObjectInstance::write_pointer(std::byte* value)
{
	require_kind(kinds_of({ ValueKind::Reference, ValueKind::String, ValueKind::Array, ValueKind::Integer, ValueKind::NativeInteger }), "write_pointer()");

	std::memcpy(m_memory, &value, sizeof(std::byte*));
}

void ObjectInstance::write_length(std::int64_t value)
{
	require_kind(kinds_of({ ValueKind::String, ValueKind::Array }), "write_length()");

	std::memcpy(m_memory, &value, sizeof(std::int64_t));
}
