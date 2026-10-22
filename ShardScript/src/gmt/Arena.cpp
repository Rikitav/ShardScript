#include <gmt/Arena.hpp>
#include <gmt/Allocator.hpp>

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
		gmt::mem_free(chunk);
}

void Arena::grow(std::size_t required)
{
	std::size_t lastSize = m_chunkSizes.empty() ? 0 : m_chunkSizes.back();
	std::size_t chunkSize = lastSize != 0 ? lastSize : initial_capacity;
	while (m_capacity + chunkSize < required)
		chunkSize *= 2;

	m_chunks.push_back(static_cast<std::byte*>(gmt::mem_alloc(chunkSize)));
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
