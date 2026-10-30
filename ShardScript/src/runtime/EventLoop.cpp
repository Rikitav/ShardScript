#include "EventLoop.hpp"

#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>

#include <uv.h>

#include <algorithm>

using namespace shard;

namespace
{
	struct TimerRequest
	{
		uv_timer_t handle{};
		EventLoop::TimerCallback callback = nullptr;
		EventLoop::TimerCallback cancel = nullptr;
		void* context = nullptr;
		bool cancelled = false;
		bool fired = false;
	};
}

struct EventLoop::Impl
{
	uv_loop_t loop{};
	gmt::vector<TimerRequest*> timers;
};

namespace
{
	void remove_timer(EventLoop::Impl* owner, TimerRequest* request)
	{
		auto choice = std::find(owner->timers.begin(), owner->timers.end(), request);
		if (choice != owner->timers.end())
			owner->timers.erase(choice);
	}

	void timer_close_cb(uv_handle_t* handle)
	{
		auto* request = static_cast<TimerRequest*>(handle->data);
		auto* owner = static_cast<EventLoop::Impl*>(handle->loop->data);
		remove_timer(owner, request);

		if (request->cancelled && !request->fired && request->cancel != nullptr)
			request->cancel(request->context);

		gmt::mem_free(request);
	}

	void timer_cb(uv_timer_t* handle)
	{
		auto* request = static_cast<TimerRequest*>(handle->data);

		// the handle is closed before the callback returns; completion only
		// queues work, so the loop never re-enters the VM here. Removing the
		// request from the live list first keeps the loop teardown from
		// cancelling a context the callback is about to release.
		request->fired = true;
		auto* owner = static_cast<EventLoop::Impl*>(handle->loop->data);
		remove_timer(owner, request);

		uv_timer_stop(handle);
		uv_close(reinterpret_cast<uv_handle_t*>(handle), &timer_close_cb);

		if (!request->cancelled)
			request->callback(request->context);
	}
}

EventLoop::EventLoop()
	: m_impl(static_cast<Impl*>(gmt::mem_alloc(sizeof(Impl))))
{
	new (m_impl) Impl();
	uv_loop_init(&m_impl->loop);
	m_impl->loop.data = m_impl;
}

EventLoop::~EventLoop()
{
	for (TimerRequest* request : m_impl->timers)
	{
		request->cancelled = true;
		uv_timer_stop(&request->handle);

		if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&request->handle)))
			uv_close(reinterpret_cast<uv_handle_t*>(&request->handle), &timer_close_cb);
	}

	// drain the close callbacks so uv_loop_close does not report busy handles
	uv_run(&m_impl->loop, UV_RUN_DEFAULT);
	uv_loop_close(&m_impl->loop);

	m_impl->~Impl();
	gmt::mem_free(m_impl);
	m_impl = nullptr;
}

bool EventLoop::run_once(bool block)
{
	const uv_run_mode mode = block ? UV_RUN_ONCE : UV_RUN_NOWAIT;
	return uv_run(&m_impl->loop, mode) != 0;
}

bool EventLoop::is_alive() const
{
	return uv_loop_alive(&m_impl->loop) != 0;
}

void EventLoop::stop()
{
	uv_stop(&m_impl->loop);
}

void EventLoop::schedule_timer(std::uint64_t milliseconds, TimerCallback callback, void* context, TimerCallback cancel)
{
	auto* request = static_cast<TimerRequest*>(gmt::mem_alloc(sizeof(TimerRequest)));
	new (request) TimerRequest();

	request->callback = callback;
	request->cancel = cancel;
	request->context = context;
	request->handle.data = request;

	uv_timer_init(&m_impl->loop, &request->handle);
	m_impl->timers.push_back(request);

	uv_timer_start(&request->handle, &timer_cb, milliseconds, 0);
}
