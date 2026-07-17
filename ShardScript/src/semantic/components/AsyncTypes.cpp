#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/TypeShape.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/EventLoop.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

using namespace shard;

namespace
{
	static inline void inherit_size(FieldSymbol* field)
	{
		TypeSymbol* parent = static_cast<TypeSymbol*>(field->Parent);
		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			field->SlotIndex = parent->NextSlotIndex++;

		field->MemoryBytesOffset = parent->MemoryBytesSize;
		parent->MemoryBytesSize += field->ReturnType->GetInlineSize();
	}
}

// ------------------------------------------------------------------------
// Task (non-generic)
// ------------------------------------------------------------------------
static ObjectInstance* shard_async_Task_MoveNext(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);
	return nullptr;
}

static ObjectInstance* shard_async_Task_IsCompleted_get(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	AsyncState state = GetTaskState(task, CLASS_TASK_StateField);
	return context.Collector.FromValue(state != AsyncState::PENDING);
}

static ObjectInstance* shard_async_Task_GetResult(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	if (GetTaskState(task, CLASS_TASK_StateField) == AsyncState::FAULTED)
	{
		ObjectInstance* exception = task->GetField(CLASS_TASK_ExceptionField->SlotIndex);
		if (exception != nullptr && exception != GarbageCollector::NullInstance)
		{
			if (context.Frame != nullptr)
			{
				exception->IncrementReference();
				context.Frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				context.Frame->InterruptionRegister = exception;
				context.Frame->CurrentException = exception;
				return GarbageCollector::NullInstance;
			}
		}
	}

	// Non-generic Task has no result.
	return GarbageCollector::NullInstance;
}

static ObjectInstance* shard_async_Task_OnCompleted(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ObjectInstance* continuation = context.Args[1];

	task->SetField(CLASS_TASK_ContinuationField->SlotIndex, continuation);

	AsyncState state = GetTaskState(task, CLASS_TASK_StateField);
	if (state != AsyncState::PENDING)
	{
		ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);
	}

	return nullptr;
}

static ObjectInstance* shard_async_Task_InternalRoot(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	context.Domain.GetEventLoop().RootTask(task);
	return nullptr;
}

static ObjectInstance* shard_async_Task_Complete(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];

	SetTaskState(task, CLASS_TASK_StateField, AsyncState::COMPLETED, context.Collector);
	ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

	// Release the factory root. Tasks created by Task.Delay carry a second
	// root that is released by the timer callback.
	context.Domain.GetEventLoop().UnrootTask(task);

	return nullptr;
}

static ObjectInstance* shard_async_Task_Delay(const CallState& context) noexcept
{
	std::int64_t milliseconds = context.Args[0]->AsInteger();

	ObjectInstance* task = context.Collector.AllocateInstance(CLASS_TASK);
	SetTaskState(task, CLASS_TASK_StateField, AsyncState::PENDING, context.Collector);

	context.Domain.GetEventLoop().RootTask(task);

	uv_timer_t* timer = new uv_timer_t;
	uv_timer_init(context.Domain.GetEventLoop().GetLoop(), timer);

	DelayState* state = new DelayState
	{
		&context.Domain,
		task,
		CLASS_TASK_StateField,
		CLASS_TASK_ContinuationField,
		TRAIT_ASYNCSTATE_MoveNext
	};

	timer->data = state;

	uv_timer_start(timer, [](uv_timer_t* handle)
	{
		DelayState* state = static_cast<DelayState*>(handle->data);
		ObjectInstance* task = state->Task;
		ApplicationDomain* domain = state->Domain;

		SetTaskState(task, state->StateField, AsyncState::COMPLETED, domain->GetGarbageCollector());

		uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* closed)
		{
			delete reinterpret_cast<uv_timer_t*>(closed);
		});

		ResumeContinuation(task, state->ContinuationField, state->MoveNextMethod, *domain);

		domain->GetEventLoop().UnrootTask(task);
		delete state;
	}, static_cast<std::uint64_t>(milliseconds), 0);

	return task;
}

static ObjectInstance* shard_async_Task_SetException(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ObjectInstance* exception = context.Args[1];

	SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, context.Collector);
	task->SetField(CLASS_TASK_ExceptionField->SlotIndex, exception);

	ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

	// Release the factory root.
	context.Domain.GetEventLoop().UnrootTask(task);

	return nullptr;
}

static ObjectInstance* shard_async_Task_Wait(const CallState& context)
{
	ObjectInstance* task = context.Args[0];
	EventLoop& loop = context.Domain.GetEventLoop();

	while (GetTaskState(task, CLASS_TASK_StateField) == AsyncState::PENDING)
		loop.RunOnce();

	if (GetTaskState(task, CLASS_TASK_StateField) == AsyncState::FAULTED)
	{
		ObjectInstance* exception = task->GetField(CLASS_TASK_ExceptionField->SlotIndex);
		if (exception != nullptr && exception != GarbageCollector::NullInstance)
		{
			CallStackFrame* caller = context.Frame != nullptr ? context.Frame->PreviousFrame : nullptr;
			if (caller != nullptr)
			{
				exception->IncrementReference();
				caller->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				caller->InterruptionRegister = exception;
				caller->CurrentException = exception;
				return nullptr;
			}
		}

		throw std::runtime_error("Task faulted");
	}

	return nullptr;
}

static ObjectInstance* shard_async_Task_GetAwaiter(const CallState& context) noexcept
{
	return context.Args[0];
}

// ------------------------------------------------------------------------
// Task<T>
// ------------------------------------------------------------------------
static ObjectInstance* shard_async_ValueTask_MoveNext(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);
	return nullptr;
}

static ObjectInstance* shard_async_ValueTask_IsCompleted_get(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	AsyncState state = GetTaskState(task, CLASS_VALUETASK_StateField);
	return context.Collector.FromValue(state != AsyncState::PENDING);
}

static ObjectInstance* shard_async_ValueTask_GetResult(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	if (GetTaskState(task, CLASS_VALUETASK_StateField) == AsyncState::FAULTED)
	{
		ObjectInstance* exception = task->GetField(CLASS_VALUETASK_ExceptionField->SlotIndex);
		if (exception != nullptr && exception != GarbageCollector::NullInstance)
		{
			if (context.Frame != nullptr)
			{
				exception->IncrementReference();
				context.Frame->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				context.Frame->InterruptionRegister = exception;
				context.Frame->CurrentException = exception;
				return GarbageCollector::NullInstance;
			}
		}
	}

	return task->GetField(CLASS_VALUETASK_ResultField->SlotIndex);
}

static ObjectInstance* shard_async_ValueTask_OnCompleted(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ObjectInstance* continuation = context.Args[1];

	task->SetField(CLASS_VALUETASK_ContinuationField->SlotIndex, continuation);

	AsyncState state = GetTaskState(task, CLASS_VALUETASK_StateField);
	if (state != AsyncState::PENDING)
	{
		ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);
	}

	return nullptr;
}

static ObjectInstance* shard_async_ValueTask_GetAwaiter(const CallState& context) noexcept
{
	return context.Args[0];
}

static ObjectInstance* shard_async_ValueTask_FromResult(const CallState& context) noexcept
{
	ObjectInstance* result = context.Args[0];
	ObjectInstance* task = context.Collector.AllocateGeneric(CLASS_VALUETASK, { const_cast<TypeSymbol*>(result->getInfo()) });

	SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::COMPLETED, context.Collector);
	task->SetField(CLASS_VALUETASK_ResultField->SlotIndex, result);

	return task;
}

static ObjectInstance* shard_async_ValueTask_SetException(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ObjectInstance* exception = context.Args[1];

	SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, context.Collector);
	task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, exception);

	ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

	context.Domain.GetEventLoop().UnrootTask(task);

	return nullptr;
}

static ObjectInstance* shard_async_ValueTask_SetResult(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	ObjectInstance* result = context.Args[1];

	SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::COMPLETED, context.Collector);
	task->SetField(CLASS_VALUETASK_ResultField->SlotIndex, result);

	ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

	context.Domain.GetEventLoop().UnrootTask(task);

	return nullptr;
}

static ObjectInstance* shard_async_ValueTask_InternalRoot(const CallState& context) noexcept
{
	ObjectInstance* task = context.Args[0];
	context.Domain.GetEventLoop().RootTask(task);
	return nullptr;
}

static ObjectInstance* shard_async_ValueTask_Wait(const CallState& context)
{
	ObjectInstance* task = context.Args[0];
	EventLoop& loop = context.Domain.GetEventLoop();

	while (GetTaskState(task, CLASS_VALUETASK_StateField) == AsyncState::PENDING)
		loop.RunOnce();

	if (GetTaskState(task, CLASS_VALUETASK_StateField) == AsyncState::FAULTED)
	{
		ObjectInstance* exception = task->GetField(CLASS_VALUETASK_ExceptionField->SlotIndex);
		if (exception != nullptr && exception != GarbageCollector::NullInstance)
		{
			CallStackFrame* caller = context.Frame != nullptr ? context.Frame->PreviousFrame : nullptr;
			if (caller != nullptr)
			{
				exception->IncrementReference();
				caller->InterruptionReason = FrameInterruptionReason::ExceptionRaised;
				caller->InterruptionRegister = exception;
				caller->CurrentException = exception;
				return nullptr;
			}
		}
		throw std::runtime_error("ValueTask faulted");
	}

	return nullptr;
}

void SymbolTable::ResolveAsyncTypes(SymbolTable* globalTable)
{
	SymbolFactory factory(globalTable);
	SymbolBuilder<NamespaceSymbol> asyncNs(globalTable, L"async");

	// -------------------------------------------------------------------------
	// interface IAsyncState
	// -------------------------------------------------------------------------
	{
		SymbolBuilder<InterfaceSymbol> builder = asyncNs.AddInterface(L"IAsyncState");
		SymbolTable::StandardTypes::IAsyncState = builder
			.DeclareGlobal();;

		SymbolTable::StandardTypes::IAsyncState_MoveNext = builder
			.AddMethod(L"MoveNext", TYPE_VOID, LINK_INSTANCE);

		SymbolTable::StandardTypes::IAsyncState_MoveNext->IsAbstract = true;
	}

	// -------------------------------------------------------------------------
	// interface IAwaiter
	// -------------------------------------------------------------------------
	{
		SymbolBuilder<InterfaceSymbol> builder = asyncNs.AddInterface(L"IAwaiter");
		SymbolTable::StandardTypes::IAwaiter = builder
			.DeclareGlobal();

		SymbolTable::StandardTypes::IAwaiter_IsCompleted = builder
			.AddMethod(L"IsCompleted", TYPE_BOOL, LINK_INSTANCE);

		SymbolTable::StandardTypes::IAwaiter_OnCompleted = builder
			.AddMethod(L"OnCompleted", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"continuation", SymbolTable::StandardTypes::IAsyncState);

		SymbolTable::StandardTypes::IAwaiter_GetResult = builder
			.AddMethod(L"GetResult", TYPE_ANY, LINK_INSTANCE);

		SymbolTable::StandardTypes::IAwaiter_OnCompleted->IsAbstract = true;
		SymbolTable::StandardTypes::IAwaiter_IsCompleted->IsAbstract = true;
		SymbolTable::StandardTypes::IAwaiter_GetResult->IsAbstract = true;
	}

	// -------------------------------------------------------------------------
	// interface IAwaitable
	// -------------------------------------------------------------------------
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IAwaitable", asyncNs);
		SymbolTable::StandardTypes::IAwaitable = builder.DeclareGlobal();

		SymbolTable::StandardTypes::IAwaitable_GetAwaiter = builder
			.AddMethod(L"GetAwaiter", SymbolTable::StandardTypes::IAwaiter, LINK_INSTANCE);

		SymbolTable::StandardTypes::IAwaitable_GetAwaiter->IsAbstract = true;
	}

	// -------------------------------------------------------------------------
	// class Task
	// -------------------------------------------------------------------------
	{
		SymbolBuilder<ClassSymbol> builder(globalTable, L"Task", asyncNs);
		SymbolTable::StandardTypes::Task = builder
			.Implements(SymbolTable::StandardTypes::IAsyncState)
			.Implements(SymbolTable::StandardTypes::IAwaitable)
			.Implements(SymbolTable::StandardTypes::IAwaiter)
			.DeclareGlobal();

		SymbolTable::StandardTypes::Task_StateField = builder.AddField(L"_state", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);
		SymbolTable::StandardTypes::Task_ContinuationField = builder.AddField(L"_continuation", SymbolTable::StandardTypes::IAsyncState, LINK_INSTANCE, ACS_PRIVATE);
		SymbolTable::StandardTypes::Task_ExceptionField = builder.AddField(L"_exception", TRAIT_THROWABLE, LINK_INSTANCE, ACS_PRIVATE);

		inherit_size(SymbolTable::StandardTypes::Task_StateField);
		inherit_size(SymbolTable::StandardTypes::Task_ContinuationField);
		inherit_size(SymbolTable::StandardTypes::Task_ExceptionField);

		SymbolTable::StandardTypes::Task->LayoutingState = TypeLayoutingState::Visited;

		builder.AddInit();

		SymbolTable::StandardTypes::Task_MoveNext = builder.AddMethod(L"MoveNext", TYPE_VOID, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAsyncState_MoveNext)
			.SetCallback(&shard_async_Task_MoveNext);

		SymbolTable::StandardTypes::Task_IsCompleted = builder.AddProperty(L"IsCompleted", TYPE_BOOL, LINK_INSTANCE)
			.AddGetter()
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_IsCompleted)
			.SetCallback(&shard_async_Task_IsCompleted_get);

		SymbolTable::StandardTypes::Task_GetAwaiter = builder.AddMethod(L"GetAwaiter", SymbolTable::StandardTypes::IAwaiter, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaitable_GetAwaiter)
			.SetCallback(&shard_async_Task_GetAwaiter);

		SymbolTable::StandardTypes::Task_GetResult = builder.AddMethod(L"GetResult", TYPE_ANY, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_GetResult)
			.SetCallback(&shard_async_Task_GetResult);

		SymbolTable::StandardTypes::Task_OnCompleted = builder.AddMethod(L"OnCompleted", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"continuation", SymbolTable::StandardTypes::IAsyncState)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_OnCompleted)
			.SetCallback(&shard_async_Task_OnCompleted);

		SymbolTable::StandardTypes::Task_Complete = builder.AddMethod(L"Complete", TYPE_VOID, LINK_INSTANCE)
			.SetCallback(&shard_async_Task_Complete);

		SymbolTable::StandardTypes::Task_SetException = builder.AddMethod(L"SetException", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"exception", TRAIT_THROWABLE)
			.SetCallback(&shard_async_Task_SetException);

		SymbolTable::StandardTypes::Task_InternalRoot = builder.AddMethod(L"InternalRoot", TYPE_VOID, LINK_STATIC)
			.AddParameter(L"task", SymbolTable::StandardTypes::Task)
			.SetCallback(&shard_async_Task_InternalRoot);

		SymbolTable::StandardTypes::Task_Delay = builder.AddMethod(L"Delay", SymbolTable::StandardTypes::Task, LINK_STATIC)
			.AddParameter(L"milliseconds", TYPE_INT)
			.SetCallback(&shard_async_Task_Delay);

		SymbolTable::StandardTypes::Wait_Task = builder.AddMethod(L"Wait", TYPE_VOID, LINK_STATIC)
			.AddParameter(L"task", SymbolTable::StandardTypes::Task)
			.SetCallback(&shard_async_Task_Wait);

		builder.AddMethod(L"Wait", TYPE_VOID, LINK_INSTANCE)
			.SetCallback(&shard_async_Task_Wait);
	}

	// -------------------------------------------------------------------------
	// class ValueTask<T>
	// -------------------------------------------------------------------------
	{
		SymbolBuilder<ClassSymbol> builder(globalTable, L"ValueTask", asyncNs);
		SymbolTable::StandardTypes::ValueTask = builder
			.Implements(SymbolTable::StandardTypes::IAsyncState)
			.Implements(SymbolTable::StandardTypes::IAwaitable)
			.Implements(SymbolTable::StandardTypes::IAwaiter)
			.DeclareGlobal();

		SymbolTable::StandardTypes::ValueTask_T = builder.AddTypeParameter(L"T");

		GenericTypeSymbol* valueTaskOfT = factory.GenericType(
			SymbolTable::StandardTypes::ValueTask, { { L"T", SymbolTable::StandardTypes::ValueTask_T } });

		SymbolTable::StandardTypes::ValueTask_StateField = builder.AddField(L"_state", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);
		SymbolTable::StandardTypes::ValueTask_ResultField = builder.AddField(L"_result", SymbolTable::StandardTypes::ValueTask_T, LINK_INSTANCE, ACS_PRIVATE);
		SymbolTable::StandardTypes::ValueTask_ContinuationField = builder.AddField(L"_continuation", SymbolTable::StandardTypes::IAsyncState, LINK_INSTANCE, ACS_PRIVATE);
		SymbolTable::StandardTypes::ValueTask_ExceptionField = builder.AddField(L"_exception", TRAIT_THROWABLE, LINK_INSTANCE, ACS_PRIVATE);

		inherit_size(SymbolTable::StandardTypes::ValueTask_StateField);
		inherit_size(SymbolTable::StandardTypes::ValueTask_ResultField);
		inherit_size(SymbolTable::StandardTypes::ValueTask_ContinuationField);
		inherit_size(SymbolTable::StandardTypes::ValueTask_ExceptionField);

		SymbolTable::StandardTypes::ValueTask->LayoutingState = TypeLayoutingState::Visited;

		SymbolTable::StandardTypes::ValueTask_IsCompleted_get = builder.AddProperty(L"IsCompleted", TYPE_BOOL, LINK_INSTANCE)
			.AddGetter()
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_IsCompleted)
			.SetCallback(&shard_async_ValueTask_IsCompleted_get);

		SymbolTable::StandardTypes::ValueTask_Result_get = builder.AddProperty(L"Result", SymbolTable::StandardTypes::ValueTask_T, LINK_INSTANCE)
			.AddGetter()
			.SetCallback(&shard_async_ValueTask_GetResult);

		builder.AddInit();

		SymbolTable::StandardTypes::ValueTask_MoveNext = builder.AddMethod(L"MoveNext", TYPE_VOID, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAsyncState_MoveNext)
			.SetCallback(&shard_async_ValueTask_MoveNext);

		SymbolTable::StandardTypes::ValueTask_GetAwaiter = builder.AddMethod(L"GetAwaiter", SymbolTable::StandardTypes::IAwaiter, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaitable_GetAwaiter)
			.SetCallback(&shard_async_ValueTask_GetAwaiter);

		SymbolTable::StandardTypes::ValueTask_GetResult = builder.AddMethod(L"GetResult", SymbolTable::StandardTypes::ValueTask_T, LINK_INSTANCE)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_GetResult)
			.SetCallback(&shard_async_ValueTask_GetResult);

		SymbolTable::StandardTypes::ValueTask_OnCompleted = builder.AddMethod(L"OnCompleted", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"continuation", SymbolTable::StandardTypes::IAsyncState)
			.IsImplementationOf(SymbolTable::StandardTypes::IAwaiter_OnCompleted)
			.SetCallback(&shard_async_ValueTask_OnCompleted);

		SymbolTable::StandardTypes::ValueTask_InternalRoot = builder.AddMethod(L"InternalRoot", TYPE_VOID, LINK_STATIC)
			.AddParameter(L"task", TYPE_ANY)
			.SetCallback(&shard_async_ValueTask_InternalRoot);

		SymbolTable::StandardTypes::ValueTask_FromResult = builder.AddMethod(L"FromResult", valueTaskOfT, LINK_STATIC)
			.AddParameter(L"value", SymbolTable::StandardTypes::ValueTask_T)
			.SetCallback(&shard_async_ValueTask_FromResult);

		SymbolTable::StandardTypes::ValueTask_SetResult = builder.AddMethod(L"SetResult", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"value", SymbolTable::StandardTypes::ValueTask_T)
			.SetCallback(&shard_async_ValueTask_SetResult);

		SymbolTable::StandardTypes::ValueTask_SetException = builder.AddMethod(L"SetException", TYPE_VOID, LINK_INSTANCE)
			.AddParameter(L"exception", TRAIT_THROWABLE)
			.SetCallback(&shard_async_ValueTask_SetException);

		SymbolTable::StandardTypes::Wait_ValueTask = builder.AddMethod(L"Wait", TYPE_VOID, LINK_STATIC)
			.AddParameter(L"task", valueTaskOfT)
			.SetCallback(&shard_async_ValueTask_Wait);

		builder.AddMethod(L"Wait", TYPE_VOID, LINK_INSTANCE)
			.SetCallback(&shard_async_ValueTask_Wait);
	}
}
