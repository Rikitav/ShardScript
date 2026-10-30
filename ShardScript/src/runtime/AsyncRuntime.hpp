#pragma once
#include <shard/Definitions.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <gmt/Containers.hpp>

#include <cstddef>
#include <cstdint>

namespace shard
{
	class EventLoop;
	class VirtualMachine;
	class StructSymbol;

	enum class TaskStatus
	{
		Pending = 0,
		Completed = 1,
		Faulted = 2
	};

	struct TaskContinuation
	{
		ObjectInstance continuation;
		ObjectInstance resultState;
	};

	struct ContinuationJob
	{
		ObjectInstance continuation;
		ObjectInstance sourceState;
		ObjectInstance resultState;
	};

	// Runtime bookkeeping for one '__TaskState' instance. The script-facing
	// Task wrapper only carries a reference to the state; everything the VM
	// needs to resume waiters lives here.
	struct TaskRuntime
	{
		ObjectInstance state;
		StructSymbol* taskType = nullptr;
		CallStackFrame* child = nullptr;	// async call frame, null for externally-completed tasks
		CallStackFrame* parent = nullptr;	// frame that launched the async call
		bool frameOwned = false;
		bool completed = false;
		bool observed = false;
		TaskStatus status = TaskStatus::Pending;
		ObjectInstance result;
		ObjectInstance exception;
		gmt::vector<CallStackFrame*> waiters;
		gmt::vector<TaskContinuation> continuations;
	};

	// All async state owned by the VM. Kept out of the public header so libuv
	// and the task bookkeeping stay implementation details.
	struct AsyncRuntime
	{
		explicit AsyncRuntime(VirtualMachine& virtualMachine);
		~AsyncRuntime();

		AsyncRuntime(const AsyncRuntime&) = delete;
		AsyncRuntime& operator=(const AsyncRuntime&) = delete;

		VirtualMachine& vm;
		gmt::unique_ptr<EventLoop> eventLoop;
		gmt::unordered_map<std::byte*, TaskRuntime> tasks;
		gmt::vector<CallStackFrame*> runnable;
		std::size_t runnableHead = 0;
		gmt::vector<ContinuationJob> pendingContinuations;
	};
}
