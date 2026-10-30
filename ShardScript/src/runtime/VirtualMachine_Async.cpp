#include <shard/runtime/VirtualMachine.hpp>

#include <shard/runtime/CallStackFrame.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include "AsyncRuntime.hpp"
#include "EventLoop.hpp"

#include <shard/compilation/ByteCodeDecoder.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/pragmatic/FrameLayout.hpp>
#include <shard/pragmatic/PragmaticModel.hpp>
#include <shard/pragmatic/TypeShape.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>

#include <algorithm>
#include <cstring>

using namespace shard;

namespace
{
	struct DelayContext
	{
		VirtualMachine* vm;
		ObjectInstance state;
	};

	// the script-visible Task wrapper stores its state reference in slot 0
	ObjectInstance task_state_of(ObjectInstance task)
	{
		if (task == nullptr || task.get_shape() == nullptr)
			return ObjectInstance::null();

		ObjectInstance slot = task.get_field(0u);
		const TypeShape* shape = slot.get_shape();
		if ((shape == nullptr) || !shape->is_reference())
			return ObjectInstance::null();

		std::byte* payload = slot.as_pointer();
		if (payload == nullptr)
			return ObjectInstance::null();

		return ObjectInstance(shape, payload);
	}

	ObjectInstance state_from_payload(std::byte* payload)
	{
		if ((payload == nullptr) || !GarbageCollector::is_heap_pointer(payload))
			return ObjectInstance::null();

		const TypeShape* shape = GarbageCollector::get_header(payload)->shape;
		return ObjectInstance(shape, payload);
	}

	int read_task_status(ObjectInstance state)
	{
		if (state == nullptr)
			return static_cast<int>(TaskStatus::Faulted);

		return static_cast<int>(state.get_field(0u).as_integer());
	}

	bool state_is_generic(ObjectInstance state)
	{
		return state != nullptr && state.get_shape() != nullptr && state.get_shape()->get_slot_count() >= 3;
	}

	ObjectInstance read_task_exception(ObjectInstance state)
	{
		if (state == nullptr)
			return ObjectInstance::null();

		ObjectInstance slot = state.get_field(1u);
		const TypeShape* shape = slot.get_shape();
		if ((shape == nullptr) || !shape->is_reference())
			return ObjectInstance::null();

		std::byte* payload = slot.as_pointer();
		if (payload == nullptr)
			return ObjectInstance::null();

		// the slot is typed 'IThrowable'; catch matching needs the concrete
		// exception type carried by the heap header
		if (GarbageCollector::is_heap_pointer(payload))
			shape = GarbageCollector::get_header(payload)->shape;

		return ObjectInstance(shape, payload);
	}

	ObjectInstance read_task_result(ObjectInstance state)
	{
		if (!state_is_generic(state))
			return ObjectInstance::null();

		ObjectInstance slot = state.get_field(2u);
		const TypeShape* shape = slot.get_shape();
		if ((shape != nullptr) && shape->is_reference())
		{
			std::byte* payload = slot.as_pointer();
			return payload != nullptr ? ObjectInstance(shape, payload) : ObjectInstance::null();
		}

		// primitive results stay inline in the rooted state payload
		return slot;
	}

	void push_task_result(CallStackFrame* frame, ObjectInstance state)
	{
		if (!state_is_generic(state))
			return;

		ObjectInstance slot = state.get_field(2u);
		const TypeShape* shape = slot.get_shape();
		if ((shape != nullptr) && shape->is_reference())
		{
			std::byte* payload = slot.as_pointer();
			frame->push_entry(shape, true, &payload, sizeof(std::byte*));
			return;
		}

		if (shape != nullptr)
			frame->push_entry(shape, false, slot.get_memory(), shape->get_size());
	}

	StructSymbol* task_type_for_state(SymbolTable& table, ObjectInstance state)
	{
		SyntaxSymbol* plainTask = table.get_standard_type(StandardTypes::Task);
		SyntaxSymbol* plainState = table.get_standard_type(StandardTypes::TaskState);
		SyntaxSymbol* genericState = table.get_standard_type(StandardTypes::TaskState_T);
		SyntaxSymbol* genericTask = table.get_standard_type(StandardTypes::Task_T);

		StructSymbol* base = state.get_shape()->get_base_type();
		if (base == plainState)
			return static_cast<StructSymbol*>(plainTask);

		if ((base != nullptr) && base->get_kind() == SyntaxKind::GenericType)
		{
			GenericTypeSymbol* construction = static_cast<GenericTypeSymbol*>(base);
			StructSymbol* underlaying = construction->get_underlaying_type();
			if ((underlaying == genericState) && !underlaying->type_parameters.empty())
			{
				StructSymbol* argument = construction->substitute_type_parameter(underlaying->type_parameters.front());
				bool hasArgument = argument != nullptr;
				bool hasGenericTask = !(genericTask == nullptr) && genericTask->is_type();
				if (hasArgument && hasGenericTask)
				{
					StructSymbol* arguments[1] = { argument };
					return table.find_or_create_generic_type(static_cast<StructSymbol*>(genericTask), std::span<StructSymbol* const>(arguments, 1));
				}
			}
		}

		return static_cast<StructSymbol*>(plainTask);
	}

	void delay_timer_elapsed(void* context)
	{
		auto* delay = static_cast<DelayContext*>(context);
		delay->vm->complete_task_state(delay->state, ObjectInstance::null());
		delay->vm->get_collector().remove_root(delay->state);
		gmt::mem_free(delay);
	}

	void delay_timer_cancelled(void* context)
	{
		auto* delay = static_cast<DelayContext*>(context);
		delay->vm->get_collector().remove_root(delay->state);
		gmt::mem_free(delay);
	}
}

AsyncRuntime::AsyncRuntime(VirtualMachine& virtualMachine)
	: vm(virtualMachine)
	, eventLoop(gmt::make_unique<EventLoop>())
{ }

AsyncRuntime::~AsyncRuntime() = default;

ObjectInstance VirtualMachine::create_task_wrapper(ObjectInstance state)
{
	if (state == nullptr)
		return ObjectInstance::null();

	StructSymbol* taskType = task_type_for_state(m_model.get_table(), state);
	const TypeShape* shape = resolve_shape(taskType);
	ObjectInstance wrapper = m_collector.allocate_instance(shape);
	wrapper.set_field(0u, state);
	return wrapper;
}

TaskRuntime& VirtualMachine::ensure_task_record(ObjectInstance state)
{
	std::byte* key = state.get_memory();

	auto existing = m_async->tasks.find(key);
	if (existing != m_async->tasks.end())
		return existing->second;

	TaskRuntime record{};
	record.state = state;
	record.taskType = task_type_for_state(m_model.get_table(), state);
	record.status = static_cast<TaskStatus>(read_task_status(state));
	record.completed = record.status != TaskStatus::Pending;

	// waiters need the state to survive while they are suspended
	m_collector.add_root(state);

	return m_async->tasks.emplace(key, std::move(record)).first->second;
}

void VirtualMachine::complete_task_status(ObjectInstance state, int status, ObjectInstance result, ObjectInstance exception)
{
	if (state == nullptr)
		return;

	TaskRuntime& record = ensure_task_record(state);
	if (record.completed)
		return;

	state.get_field(0u).write_integer(status);
	if (static_cast<TaskStatus>(status) == TaskStatus::Faulted)
		state.set_field(1u, exception);
	else if (state_is_generic(state))
		state.set_field(2u, result);

	record.completed = true;
	record.status = static_cast<TaskStatus>(status);
	record.result = result;
	record.exception = exception;

	gmt::vector<CallStackFrame*> waiters = std::move(record.waiters);
	record.waiters.clear();
	for (CallStackFrame* waiter : waiters)
	{
		waiter->set_awaited_state(state.get_memory());
		enqueue_runnable(waiter);
	}

	gmt::vector<TaskContinuation> continuations = std::move(record.continuations);
	record.continuations.clear();
	for (const TaskContinuation& continuation : continuations)
	{
		m_collector.add_root(continuation.continuation);
		m_collector.add_root(state);
		m_collector.add_root(continuation.resultState);
		m_async->pendingContinuations.push_back({ continuation.continuation, state, continuation.resultState });
	}

	if (continuations.empty() && !record.frameOwned)
	{
		// waiters fall back to the state fields; a job keeps the state rooted
		// through the extra roots taken above
		m_collector.remove_root(record.state);
		m_async->tasks.erase(state.get_memory());
	}
}

void VirtualMachine::complete_task_state(ObjectInstance state, ObjectInstance result)
{
	complete_task_status(state, static_cast<int>(TaskStatus::Completed), result, ObjectInstance::null());
}

void VirtualMachine::fail_task_state(ObjectInstance state, ObjectInstance exception)
{
	complete_task_status(state, static_cast<int>(TaskStatus::Faulted), ObjectInstance::null(), exception);
}

void VirtualMachine::complete_task(ObjectInstance task, ObjectInstance result)
{
	complete_task_state(task_state_of(task), result);
}

void VirtualMachine::complete_task(ObjectInstance task)
{
	complete_task_state(task_state_of(task), ObjectInstance::null());
}

void VirtualMachine::fail_task(ObjectInstance task, ObjectInstance exception)
{
	fail_task_state(task_state_of(task), exception);
}

bool VirtualMachine::is_task_completed(ObjectInstance task)
{
	ObjectInstance state = task_state_of(task);
	if (state == nullptr)
		return false;

	return read_task_status(state) != static_cast<int>(TaskStatus::Pending);
}

ObjectInstance VirtualMachine::await_task(ObjectInstance task)
{
	ObjectInstance state = task_state_of(task);
	if (state == nullptr)
	{
		raise_runtime_exception(L"await target is not a Task");
		return ObjectInstance::null();
	}

	wait_for_task(state);

	int status = read_task_status(state);
	auto record = m_async->tasks.find(state.get_memory());
	if (record != m_async->tasks.end())
		record->second.observed = true;

	if (status == static_cast<int>(TaskStatus::Faulted))
	{
		raise_exception(read_task_exception(state));
		return ObjectInstance::null();
	}

	ObjectInstance result = read_task_result(state);
	const TypeShape* shape = result.get_shape();
	if ((shape != nullptr) && !shape->is_reference())
	{
		std::memcpy(m_returnScratch, result.get_memory(), shape->get_size());
		result = ObjectInstance(shape, m_returnScratch);
	}

	return result;
}

ObjectInstance VirtualMachine::launch_async_method(MethodSymbol* method, std::span<const ObjectInstance> arguments)
{
	return invoke_async_method(method, arguments, nullptr);
}

ObjectInstance VirtualMachine::delay_task(std::int64_t milliseconds)
{
	SymbolTable& table = m_model.get_table();
	StructSymbol* stateType = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::TaskState));

	ObjectInstance state = m_collector.allocate_instance(resolve_shape(stateType));
	m_collector.add_root(state);
	ObjectInstance wrapper = create_task_wrapper(state);

	auto* delay = static_cast<DelayContext*>(gmt::mem_alloc(sizeof(DelayContext)));
	delay->vm = this;
	delay->state = state;

	std::uint64_t timeout = milliseconds > 0 ? static_cast<std::uint64_t>(milliseconds) : 0;
	m_async->eventLoop->schedule_timer(timeout, &delay_timer_elapsed, delay, &delay_timer_cancelled);

	return wrapper;
}

ObjectInstance VirtualMachine::continue_task(ObjectInstance task, ObjectInstance continuation)
{
	ObjectInstance sourceState = task_state_of(task);
	if (sourceState == nullptr)
	{
		raise_runtime_exception(L"ContinueWith target is not a Task");
		return ObjectInstance::null();
	}

	// the continuation result shares the source state shape: Task -> Task,
	// Task<T> -> Task<T>
	ObjectInstance resultState = m_collector.allocate_instance(sourceState.get_shape());
	ObjectInstance resultTask = create_task_wrapper(resultState);

	m_collector.add_root(resultState);
	m_collector.add_root(continuation);

	TaskRuntime& source = ensure_task_record(sourceState);
	source.continuations.push_back({ continuation, resultState });

	if (source.completed)
	{
		gmt::vector<TaskContinuation> continuations = std::move(source.continuations);
		source.continuations.clear();

		for (const TaskContinuation& pending : continuations)
		{
			m_collector.add_root(pending.continuation);
			m_collector.add_root(sourceState);
			m_collector.add_root(pending.resultState);
			m_async->pendingContinuations.push_back({ pending.continuation, sourceState, pending.resultState });
		}
	}

	return resultTask;
}

ObjectInstance VirtualMachine::invoke_async_method(MethodSymbol* method, std::span<const ObjectInstance> arguments, const FrameLayout* layout)
{
	SymbolTable& table = m_model.get_table();
	bool returnsVoid = is_void_method(method);

	StructSymbol* stateType = nullptr;
	StructSymbol* taskType = nullptr;

	if (returnsVoid)
	{
		stateType = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::TaskState));
		taskType = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::Task));
	}
	else
	{
		StructSymbol* returnType = method->get_return_type();
		StructSymbol* openState = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::TaskState_T));
		StructSymbol* openTask = static_cast<StructSymbol*>(table.get_standard_type(StandardTypes::Task_T));

		StructSymbol* arguments1[1] = { returnType };
		std::span<StructSymbol* const> stateArguments(arguments1, 1);

		stateType = table.find_or_create_generic_type(openState, stateArguments);
		taskType = table.find_or_create_generic_type(openTask, stateArguments);
	}

	ObjectInstance state = m_collector.allocate_instance(resolve_shape(stateType));
	m_collector.add_root(state);
	ObjectInstance wrapper = create_task_wrapper(state);

	if (layout == nullptr)
		layout = m_pragmatic.find_frame_layout(method);
	if (layout == nullptr)
		layout = &m_pragmatic.get_or_create_frame_layout(m_model, method);

	CallStackFrame* parent = m_currentFrame;
	CallStackFrame* child = CallStackFrame::create(method, layout, m_model, m_pragmatic, parent, nullptr, 0, &m_framePool);
	m_collector.register_frame(child);

	for (std::size_t i = 0; i < arguments.size(); ++i)
		child->set_local(static_cast<std::uint16_t>(i), arguments[i]);

	if (parent != nullptr)
		parent->increment_pending_children();

	std::byte* key = state.get_memory();
	child->set_async_state(key);

	TaskRuntime record{};
	record.state = state;
	record.taskType = taskType;
	record.child = child;
	record.parent = parent;
	record.frameOwned = true;
	record.status = TaskStatus::Pending;
	m_async->tasks.emplace(key, std::move(record));

	CallStackFrame* saved = m_currentFrame;
	m_currentFrame = child;
	execute_frame(child);
	m_currentFrame = saved;

	if (child->get_interruption() == InterruptionReason::Awaiting)
		return wrapper;

	finish_async_frame(child);
	return wrapper;
}

void VirtualMachine::finish_async_frame(CallStackFrame* frame)
{
	if (frame == nullptr)
		return;

	std::byte* key = frame->get_async_state();
	auto record = m_async->tasks.find(key);

	ObjectInstance ownState = (record != m_async->tasks.end()) ? record->second.state : ObjectInstance::null();
	CallStackFrame* parent = (record != m_async->tasks.end()) ? record->second.parent : nullptr;

	const bool raised = frame->get_interruption() == InterruptionReason::ExceptionRaised;
	ObjectInstance exception = raised ? frame->get_current_exception() : ObjectInstance::null();
	ObjectInstance value = ObjectInstance::null();

	if (!raised && !is_void_method(frame->get_method()))
	{
		ObjectInstance view = frame->get_return_slot();
		const TypeShape* shape = view.get_shape();

		if ((shape != nullptr) && !shape->is_reference())
		{
			std::memcpy(m_returnScratch, view.get_memory(), shape->get_size());
			value = ObjectInstance(shape, m_returnScratch);
		}
		else
		{
			value = view;
		}
	}

	bool faulted = raised;
	ObjectInstance fault = raised ? exception : take_unobserved_fault(frame);
	if (!(fault == nullptr))
	{
		faulted = true;
		exception = fault;
		value = ObjectInstance::null();
	}

	abandon_children(frame);
	m_collector.unregister_frame(frame);
	CallStackFrame::destroy(frame, &m_framePool);

	if (!(ownState == nullptr))
	{
		// the frame block is recycled now; clear the record before anything
		// walks the child frames again
		auto ownRecord = m_async->tasks.find(key);
		if (ownRecord != m_async->tasks.end())
			ownRecord->second.child = nullptr;

		complete_task_status(ownState,
			static_cast<int>(faulted ? TaskStatus::Faulted : TaskStatus::Completed),
			value,
			faulted ? exception : ObjectInstance::null());
	}

	if (parent != nullptr)
	{
		parent->decrement_pending_children();
		if (parent->get_pending_children() == 0 && parent->get_suspended_at_return())
			enqueue_runnable(parent);
	}

	prune_tasks_for_frame(frame);
}

void VirtualMachine::resume_frame(CallStackFrame* frame)
{
	if (frame == nullptr)
		return;

	if (frame->get_awaited_state() != nullptr)
	{
		deliver_await_result(frame);
	}
	else if (frame->get_suspended_at_return())
	{
		frame->set_suspended_at_return(false);

		ObjectInstance fault = take_unobserved_fault(frame);
		if (!(fault == nullptr))
		{
			frame->set_current_exception(fault);
			frame->set_interruption(InterruptionReason::ExceptionRaised);
		}
		else
		{
			frame->set_interruption(InterruptionReason::ValueReturned);
		}
	}

	CallStackFrame* saved = m_currentFrame;
	m_currentFrame = frame;
	execute_frame(frame);
	m_currentFrame = saved;

	if (frame->get_interruption() != InterruptionReason::Awaiting)
		finish_async_frame(frame);
}

void VirtualMachine::deliver_await_result(CallStackFrame* frame)
{
	std::byte* payload = frame->get_awaited_state();
	frame->set_awaited_state(nullptr);

	ObjectInstance state = state_from_payload(payload);
	if (state == nullptr)
	{
		raise_runtime_exception(L"awaited task state is no longer available");
		return;
	}

	auto record = m_async->tasks.find(payload);
	if (record != m_async->tasks.end())
		record->second.observed = true;

	int status = read_task_status(state);
	if (status == static_cast<int>(TaskStatus::Faulted))
	{
		frame->set_current_exception(read_task_exception(state));
		frame->set_interruption(InterruptionReason::ExceptionRaised);
		return;
	}

	frame->set_interruption(InterruptionReason::None);
	push_task_result(frame, state);
}

void VirtualMachine::execute_await(CallStackFrame* frame, ByteCodeDecoder& decoder)
{
	ObjectInstance task = frame->pop_entry();
	if (task == nullptr)
	{
		raise_runtime_exception(L"NullReferenceException");
		return;
	}

	ObjectInstance state = task_state_of(task);
	if (state == nullptr)
	{
		raise_runtime_exception(L"await target is not a Task");
		return;
	}

	int status = read_task_status(state);
	if (status == static_cast<int>(TaskStatus::Pending))
	{
		bool asyncFrame = frame->get_async_state() != nullptr;
		if (asyncFrame)
		{
			TaskRuntime& record = ensure_task_record(state);
			record.waiters.push_back(frame);
			frame->set_awaited_state(state.get_memory());
			frame->set_instruction_pointer(decoder.get_cursor());
			frame->set_interruption(InterruptionReason::Awaiting);
			return;
		}

		wait_for_task(state);
		status = read_task_status(state);
		if (status == static_cast<int>(TaskStatus::Pending))
			return;
	}

	auto record = m_async->tasks.find(state.get_memory());
	if (record != m_async->tasks.end())
		record->second.observed = true;

	if (status == static_cast<int>(TaskStatus::Faulted))
	{
		frame->set_current_exception(read_task_exception(state));
		frame->set_interruption(InterruptionReason::ExceptionRaised);
		return;
	}

	push_task_result(frame, state);
}

bool VirtualMachine::pump_async_once(bool block)
{
	if (m_async->runnableHead < m_async->runnable.size())
	{
		CallStackFrame* frame = m_async->runnable[m_async->runnableHead++];
		if (m_async->runnableHead == m_async->runnable.size())
		{
			m_async->runnable.clear();
			m_async->runnableHead = 0;
		}

		// abandoned frames are detached by nulling their queue entry
		if (frame == nullptr)
			return true;

		frame->set_is_runnable(false);
		resume_frame(frame);
		return true;
	}

	if (!m_async->pendingContinuations.empty())
	{
		run_pending_continuation();
		return true;
	}

	if (m_async->eventLoop->is_alive())
	{
		m_async->eventLoop->run_once(block);
		return true;
	}

	return false;
}

void VirtualMachine::wait_for_task(ObjectInstance state)
{
	while (read_task_status(state) == static_cast<int>(TaskStatus::Pending))
	{
		if (pump_async_once(true))
			continue;

		raise_runtime_exception(L"deadlock: awaited task never completed");
		return;
	}
}

void VirtualMachine::wait_for_children(CallStackFrame* frame)
{
	while (frame->get_pending_children() > 0)
	{
		if (pump_async_once(true))
			continue;

		raise_runtime_exception(L"deadlock: async children never completed");
		return;
	}
}

void VirtualMachine::enqueue_runnable(CallStackFrame* frame)
{
	if (frame == nullptr || frame->get_is_runnable())
		return;

	frame->set_is_runnable(true);
	m_async->runnable.push_back(frame);
}

void VirtualMachine::abandon_children(CallStackFrame* frame)
{
	if ((m_async == nullptr) || (frame == nullptr))
		return;

	gmt::vector<ObjectInstance> abandoned;
	for (auto& [key, record] : m_async->tasks)
	{
		if ((record.parent != frame) || (record.child == nullptr))
			continue;

		record.parent = nullptr;
		abandoned.push_back(record.state);
	}

	for (const ObjectInstance& state : abandoned)
	{
		std::byte* key = state.get_memory();
		auto record = m_async->tasks.find(key);
		if (record == m_async->tasks.end())
			continue;

		if (record->second.child != nullptr)
		{
			CallStackFrame* child = record->second.child;

			// the child may be parked in another task's waiter list or in the
			// runnable queue; detach it before the frame block is recycled
			for (auto& [otherKey, other] : m_async->tasks)
			{
				auto waiter = std::find(other.waiters.begin(), other.waiters.end(), child);
				if (waiter != other.waiters.end())
					other.waiters.erase(waiter);
			}

			for (CallStackFrame*& queued : m_async->runnable)
			{
				if (queued == child)
					queued = nullptr;
			}

			m_collector.unregister_frame(child);
			CallStackFrame::destroy(child, &m_framePool);
			record->second.child = nullptr;
		}

		if (!record->second.completed)
		{
			ObjectInstance exception = build_runtime_exception(L"async operation was abandoned");
			complete_task_status(state, static_cast<int>(TaskStatus::Faulted), ObjectInstance::null(), exception);
		}

		// a queued continuation keeps the state rooted until the job runs
		bool usedByJob = false;
		for (const ContinuationJob& job : m_async->pendingContinuations)
		{
			if (job.sourceState.get_memory() != key)
				continue;

			usedByJob = true;
			break;
		}

		if (usedByJob)
			continue;

		// the parent is gone, so nothing will prune this task later
		auto left = m_async->tasks.find(key);
		if (left != m_async->tasks.end())
		{
			m_collector.remove_root(left->second.state);
			m_async->tasks.erase(left);
		}
	}
}

ObjectInstance VirtualMachine::take_unobserved_fault(CallStackFrame* frame)
{
	for (auto& [key, record] : m_async->tasks)
	{
		if (record.parent != frame || !record.completed || record.observed)
			continue;

		if (record.status != TaskStatus::Faulted)
			continue;

		record.observed = true;
		return record.exception;
	}

	return ObjectInstance::null();
}

void VirtualMachine::prune_tasks_for_frame(CallStackFrame* frame)
{
	for (auto choice = m_async->tasks.begin(); choice != m_async->tasks.end();)
	{
		TaskRuntime& record = choice->second;
		if (record.parent != frame || !record.completed)
		{
			++choice;
			continue;
		}

		// a continuation queued for this state still needs its root
		bool usedByJob = false;
		for (const ContinuationJob& job : m_async->pendingContinuations)
		{
			if (job.sourceState.get_memory() != record.state.get_memory())
				continue;

			usedByJob = true;
			break;
		}

		if (usedByJob)
		{
			++choice;
			continue;
		}

		m_collector.remove_root(record.state);
		choice = m_async->tasks.erase(choice);
	}
}

void VirtualMachine::run_pending_continuation()
{
	ContinuationJob job = m_async->pendingContinuations.front();
	m_async->pendingContinuations.erase(m_async->pendingContinuations.begin());

	int status = read_task_status(job.sourceState);
	ObjectInstance sourceException = (status == static_cast<int>(TaskStatus::Faulted))
		? read_task_exception(job.sourceState)
		: ObjectInstance::null();
	ObjectInstance sourceResult = (status == static_cast<int>(TaskStatus::Completed))
		? read_task_result(job.sourceState)
		: ObjectInstance::null();

	ObjectInstance thrown = ObjectInstance::null();
	ObjectInstance delegate = job.continuation;

	if (delegate != nullptr && delegate.get_shape() != nullptr)
	{
		const TypeShape* boxShape = delegate.get_shape();
		if (m_collector.is_heap_backed(delegate.get_memory()))
			boxShape = GarbageCollector::get_header(delegate.get_memory())->shape;

		StructSymbol* boxType = boxShape->get_base_type();
		if ((boxType != nullptr) && !boxType->methods.empty())
		{
			MethodSymbol* target = boxType->methods.front();
			ObjectInstance arguments[1] = { delegate };
			ObjectInstance result = try_invoke_method(target, std::span<const ObjectInstance>(arguments, 1), thrown);
			(void)result;
		}
	}

	if (!(thrown == nullptr))
		fail_task_state(job.resultState, thrown);
	else if (status == static_cast<int>(TaskStatus::Faulted))
		fail_task_state(job.resultState, sourceException);
	else
		complete_task_state(job.resultState, sourceResult);

	m_collector.remove_root(job.continuation);
	m_collector.remove_root(job.sourceState);
	m_collector.remove_root(job.resultState);

	auto sourceRecord = m_async->tasks.find(job.sourceState.get_memory());
	if ((sourceRecord != m_async->tasks.end()) && !sourceRecord->second.frameOwned)
	{
		m_collector.remove_root(sourceRecord->second.state);
		m_async->tasks.erase(sourceRecord);
	}
}
