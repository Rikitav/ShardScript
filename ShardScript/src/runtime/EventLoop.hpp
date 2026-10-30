#pragma once
#include <shard/Definitions.hpp>

#include <cstdint>

namespace shard
{
	// Single-threaded libuv loop owned by the VM. The header deliberately
	// stays free of <uv.h>: libuv is an implementation detail of the core
	// library, never part of its public surface.
	class EventLoop
	{
	public:
		using TimerCallback = void(*)(void* context);

		struct Impl;

	private:
		Impl* m_impl = nullptr;

	public:
		EventLoop();
		~EventLoop();

		EventLoop(const EventLoop&) = delete;
		EventLoop& operator=(const EventLoop&) = delete;

		// runs one loop iteration; 'block' waits for the next event when the
		// loop has active handles
		bool run_once(bool block);
		bool is_alive() const;
		void stop();

		// one-shot timer; the callback runs on the loop thread and must not
		// execute managed code. 'cancel' runs instead when the timer is closed
		// before it fires
		void schedule_timer(std::uint64_t milliseconds, TimerCallback callback, void* context, TimerCallback cancel = nullptr);
	};
}
