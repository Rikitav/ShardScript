#include <shard/libframework/AsyncMutex.hpp>

#include <uv.h>

#include <system_error>

using namespace shard;

namespace
{
	constexpr std::size_t InlineStorageBytes = 64;
	static_assert(InlineStorageBytes >= sizeof(uv_mutex_t), "the inline uv_mutex_t storage is too small on this platform");

	uv_mutex_t* as_mutex(std::byte* storage)
	{
		return reinterpret_cast<uv_mutex_t*>(storage);
	}

	// The owner identity must be unique per thread, and thread_local is essential:
	// a plain function-local static is shared, so every thread would present the
	// same token. uv_thread_self() cannot serve here -- it is GetCurrentThread()
	// on Windows, a pseudo-handle every thread shares -- so the platform's real
	// thread id is used and the mutual exclusion itself stays on uv_mutex_t.
	// 0 stays reserved for "no owner".
	std::uint64_t current_thread_token()
	{
#if defined(_WIN32)
		static thread_local const std::uint64_t token = static_cast<std::uint64_t>(GetCurrentThreadId());
#else
		static thread_local const std::uint64_t token = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pthread_self()));
#endif
		return token;
	}

	[[noreturn]] void throw_deadlock(const char* message)
	{
		throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur), message);
	}

	[[noreturn]] void throw_not_owner(const char* message)
	{
		throw std::system_error(std::make_error_code(std::errc::operation_not_permitted), message);
	}
}

AsyncMutex::AsyncMutex()
{
	if (uv_mutex_init(as_mutex(m_storage)) != 0)
		throw std::system_error(std::make_error_code(std::errc::not_enough_memory), "AsyncMutex: uv_mutex_init failed");
}

AsyncMutex::~AsyncMutex()
{
	uv_mutex_destroy(as_mutex(m_storage));
}

void AsyncMutex::lock()
{
	const std::uint64_t self = current_thread_token();
	if (m_owner.load(std::memory_order_relaxed) == self)
		throw_deadlock("AsyncMutex::lock: already locked by this thread");

	uv_mutex_lock(as_mutex(m_storage));
	m_owner.store(self, std::memory_order_relaxed);
}

bool AsyncMutex::try_lock()
{
	const std::uint64_t self = current_thread_token();
	if (m_owner.load(std::memory_order_relaxed) == self)
		return false;

	if (uv_mutex_trylock(as_mutex(m_storage)) != 0)
		return false;

	m_owner.store(self, std::memory_order_relaxed);
	return true;
}

void AsyncMutex::unlock()
{
	if (m_owner.load(std::memory_order_relaxed) != current_thread_token())
		throw_not_owner("AsyncMutex::unlock: not locked by this thread");

	m_owner.store(0, std::memory_order_relaxed);
	uv_mutex_unlock(as_mutex(m_storage));
}

RecursiveAsyncMutex::RecursiveAsyncMutex()
{
	if (uv_mutex_init(as_mutex(m_storage)) != 0)
		throw std::system_error(std::make_error_code(std::errc::not_enough_memory), "RecursiveAsyncMutex: uv_mutex_init failed");
}

RecursiveAsyncMutex::~RecursiveAsyncMutex()
{
	uv_mutex_destroy(as_mutex(m_storage));
}

void RecursiveAsyncMutex::lock()
{
	const std::uint64_t self = current_thread_token();
	if (m_owner.load(std::memory_order_relaxed) == self)
	{
		++m_depth;
		return;
	}

	uv_mutex_lock(as_mutex(m_storage));
	m_owner.store(self, std::memory_order_relaxed);
	m_depth = 1;
}

bool RecursiveAsyncMutex::try_lock()
{
	const std::uint64_t self = current_thread_token();
	if (m_owner.load(std::memory_order_relaxed) == self)
	{
		++m_depth;
		return true;
	}

	if (uv_mutex_trylock(as_mutex(m_storage)) != 0)
		return false;

	m_owner.store(self, std::memory_order_relaxed);
	m_depth = 1;
	return true;
}

void RecursiveAsyncMutex::unlock()
{
	if (m_owner.load(std::memory_order_relaxed) != current_thread_token())
		throw_not_owner("RecursiveAsyncMutex::unlock: not locked by this thread");

	if (--m_depth > 0)
		return;

	m_owner.store(0, std::memory_order_relaxed);
	uv_mutex_unlock(as_mutex(m_storage));
}
