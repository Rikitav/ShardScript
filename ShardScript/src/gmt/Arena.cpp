#include <gmt/Arena.hpp>

#include <mimalloc.h>
#include <cstdlib>

using namespace gmt;

Arena::Arena() :
	m_chunks(),
	m_chunkSizes(),
	m_chunkBases(),
	m_capacity(0),
	m_cursor(0),
	m_destructors()
{ }

Arena::~Arena()
{
	for (auto it = m_destructors.rbegin(); it != m_destructors.rend(); ++it)
		it->destroy(resolve(it->offset));

	for (std::byte* chunk : m_chunks)
	{
#ifdef GMT_ARENA_NO_MIMALLOC
		::free(chunk);
#else
		mi_free(chunk);
#endif
	}
}

void* Arena::allocate(std::size_t bytes)
{
#ifdef GMT_ARENA_NO_MIMALLOC
	return ::malloc(bytes);
#else
	return mi_malloc(bytes);
#endif
}

void Arena::grow(std::size_t required)
{
	std::size_t lastSize = m_chunkSizes.empty() ? 0 : m_chunkSizes.back();
	std::size_t chunkSize = lastSize != 0 ? lastSize : initial_capacity;
	while (m_capacity + chunkSize < required)
		chunkSize *= 2;

	m_chunks.push_back(static_cast<std::byte*>(allocate(chunkSize)));
	m_chunkSizes.push_back(chunkSize);
	m_chunkBases.push_back(m_capacity);
	m_capacity += chunkSize;
}

void* Arena::resolve(std::size_t offset) const
{
	for (std::size_t i = m_chunks.size(); i-- > 0; )
	{
		if (offset < m_chunkBases[i])
			continue;

		return m_chunks[i] + (offset - m_chunkBases[i]);
	}

	return nullptr;
}
