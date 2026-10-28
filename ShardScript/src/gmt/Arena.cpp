#include <gmt/Arena.hpp>
#include <gmt/Allocator.hpp>

#include <algorithm>
#include <mutex>

using namespace gmt;

namespace
{
	struct DestructorRegistry
	{
		std::mutex mutex;
		gmt::vector<gmt::detail::destructor_info> infos;
	};

	DestructorRegistry& destructor_registry()
	{
		// deliberately leaked: arenas may outlive static destruction order
		static DestructorRegistry* registry = new DestructorRegistry();
		return *registry;
	}
}

std::uint32_t gmt::detail::register_destructor_type(void (*destroy)(void*), std::size_t size)
{
	DestructorRegistry& registry = destructor_registry();
	std::lock_guard<std::mutex> lock(registry.mutex);
	registry.infos.push_back(gmt::detail::destructor_info{ destroy, size });
	return static_cast<std::uint32_t>(registry.infos.size() - 1);
}

gmt::detail::destructor_info gmt::detail::destructor_info_at(std::uint32_t type)
{
	DestructorRegistry& registry = destructor_registry();
	std::lock_guard<std::mutex> lock(registry.mutex);
	return registry.infos[type];
}

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
	{
		const detail::destructor_info info = detail::destructor_info_at(it->type);
		std::byte* base = static_cast<std::byte*>(resolve(it->offset));

		// runs are contiguous in both offset and address space (a chunk
		// boundary starts a new record), so walk the run backwards
		for (std::uint32_t i = it->count; i > 0; --i)
			info.destroy(base + static_cast<std::size_t>(i - 1) * info.size);
	}

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
	// previous linear scan ran on every T*::operator-> and dominated
	// symbol-heavy work once the arena grew past a few chunks
	if (offset >= m_capacity)
		return nullptr;

	const auto it = std::upper_bound(m_chunkBases.begin(), m_chunkBases.end(), offset);
	const std::size_t index = static_cast<std::size_t>(it - m_chunkBases.begin()) - 1;
	return m_chunks[index] + (offset - m_chunkBases[index]);
}
