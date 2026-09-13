#include <gmt/Arena.hpp>

#include <mimalloc.h>

#include <cstring>

using namespace gmt;

Arena::Arena() :
	m_buffer(nullptr),
	m_capacity(0),
	m_cursor(0),
	m_destructors()
{ }

Arena::~Arena()
{
	for (auto it = m_destructors.rbegin(); it != m_destructors.rend(); ++it)
		it->destroy(m_buffer + it->offset);

	if (m_buffer != nullptr)
		mi_free(m_buffer);
}

void* Arena::allocate(std::size_t bytes)
{
	return mi_malloc(bytes);
}

void Arena::grow(std::size_t required)
{
	std::size_t new_capacity = m_capacity != 0 ? m_capacity : initial_capacity;
	while (new_capacity < required)
		new_capacity *= 2;

	std::byte* new_buffer = static_cast<std::byte*>(allocate(new_capacity));
	if (m_buffer != nullptr)
	{
		std::memcpy(new_buffer, m_buffer, m_cursor);
		mi_free(m_buffer);
	}

	m_buffer = new_buffer;
	m_capacity = new_capacity;
}
