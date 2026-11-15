#pragma once
#include <shard/Definitions.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace shard
{
	// Mutual exclusion for native code that the async runtime calls into:
	// concurrent collections, channels, ORM and model state, and anything else
	// that must not interleave between tasks.
	//
	// Both mutexes wrap a libuv mutex, so they pair with uv_cond_t waits on the
	// event loop, and both are Lockable: std::lock_guard, std::unique_lock,
	// std::scoped_lock and std::condition_variable_any work directly.
	//
	// Tasks all run on the loop thread, so a lock held across an await is hit by the
	// next task on the same thread: re-entry fails fast instead of hanging the VM.
	// Use RecursiveAsyncMutex where same-thread re-entry is intended.
	class SHARD_API AsyncMutex
	{
		// uv_mutex_t lives here by value; <uv.h> stays out of this header so
		// framework libraries keep building against the core's public includes
		alignas(std::max_align_t) std::byte m_storage[64] = {};
		std::atomic<std::uint64_t> m_owner{ 0 };

	public:
		AsyncMutex();
		~AsyncMutex();

		AsyncMutex(const AsyncMutex&) = delete;
		AsyncMutex& operator=(const AsyncMutex&) = delete;

		void lock();
		bool try_lock();
		void unlock();
	};

	// The recursive counterpart: the owning thread may lock again and the mutex
	// is released only when the outermost unlock runs.
	class SHARD_API RecursiveAsyncMutex
	{
		alignas(std::max_align_t) std::byte m_storage[64] = {};
		std::atomic<std::uint64_t> m_owner{ 0 };
		std::uint32_t m_depth = 0;

	public:
		RecursiveAsyncMutex();
		~RecursiveAsyncMutex();

		RecursiveAsyncMutex(const RecursiveAsyncMutex&) = delete;
		RecursiveAsyncMutex& operator=(const RecursiveAsyncMutex&) = delete;

		void lock();
		bool try_lock();
		void unlock();
	};
}
