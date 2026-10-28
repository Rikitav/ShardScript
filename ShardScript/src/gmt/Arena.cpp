#include <gmt/Arena.hpp>
#include <gmt/Allocator.hpp>

#include <algorithm>

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
	// double the previous chunk so the chunk count stays logarithmic; the
	// old code reused the previous size, so a large arena degenerated into
	// hundreds of 16 KB chunks and made resolve() linear in chunk count
	std::size_t chunkSize = m_chunkSizes.empty() ? initial_capacity : m_chunkSizes.back() * 2;
	while (m_capacity + chunkSize < required)
		chunkSize *= 2;

	m_chunks.push_back(static_cast<std::byte*>(gmt::mem_alloc(chunkSize)));
	m_chunkSizes.push_back(chunkSize);
	m_chunkBases.push_back(m_capacity);
	m_capacity += chunkSize;
}

void* Arena::resolve(std::size_t offset) const
{
	// chunk bases are sorted ascending and partition [0, capacity); the
	// previous linear scan ran on every Ref<T>::operator-> and dominated
	// symbol-heavy work once the arena grew past a few chunks
	if (offset >= m_capacity)
		return nullptr;

	const auto it = std::upper_bound(m_chunkBases.begin(), m_chunkBases.end(), offset);
	const std::size_t index = static_cast<std::size_t>(it - m_chunkBases.begin()) - 1;
	return m_chunks[index] + (offset - m_chunkBases[index]);
}
