#include <shard/runtime/ObjectInstance.hpp>

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
