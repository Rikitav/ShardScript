#include <shard/runtime/NativeAsync.hpp>

#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <uv.h>

#include <thread>
#include <unordered_map>
#include <vector>

using namespace shard;

namespace
{
    std::unordered_map<ObjectInstance*, std::function<void()>> g_nativeContinuationCallbacks;
}

namespace shard
{
    namespace detail
    {
        void AsyncScopeState::EnsureCompletedOnce()
        {
            if (completed)
                return;

            completed = true;
        }

        void AsyncScopeState::CompleteTask()
        {
            EnsureCompletedOnce();
            SetTaskState(task, CLASS_TASK_StateField, AsyncState::COMPLETED, *collector);
            task->ReleaseFrameOwner();

            ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, *domain);
            domain->GetEventLoop().UnrootTask(task);
        }

        void AsyncScopeState::FailTask(ObjectInstance* exception)
        {
            EnsureCompletedOnce();
            SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, *collector);
            task->SetField(CLASS_TASK_ExceptionField->SlotIndex, exception);
            task->ReleaseFrameOwner();

            ResumeContinuation(task, CLASS_TASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, *domain);
            domain->GetEventLoop().UnrootTask(task);
        }

        void AsyncScopeState::SetValueTaskResult(ObjectInstance* result)
        {
            EnsureCompletedOnce();
            SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::COMPLETED, *collector);
            task->SetField(CLASS_VALUETASK_ResultField->SlotIndex, result);
            task->ReleaseFrameOwner();

            ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, *domain);
            domain->GetEventLoop().UnrootTask(task);
        }

        void AsyncScopeState::FailValueTask(ObjectInstance* exception)
        {
            EnsureCompletedOnce();
            SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, *collector);
            task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, exception);
            task->ReleaseFrameOwner();

            ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, *domain);
            domain->GetEventLoop().UnrootTask(task);
        }

        void AsyncScopeState::Halt()
        {
            if (completed)
                return;
            completed = true;

            if (ActiveHandle != nullptr)
            {
                uv_close(ActiveHandle, [](uv_handle_t* closed)
                {
                    delete closed;
                });

                ActiveHandle = nullptr;
            }

            ObjectInstance* exception = CreateRuntimeException(*collector,
                L"Task halted because the virtual machine has stopped");

            if (isValueTask)
            {
                SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, *collector);
                task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, exception);
            }
            else
            {
                SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, *collector);
                task->SetField(CLASS_TASK_ExceptionField->SlotIndex, exception);
            }

            task->ReleaseFrameOwner();
            domain->GetEventLoop().UnrootTask(task);
        }

        std::shared_ptr<AsyncScopeState> CreateAsyncScopeState(const CallState& ctx, TypeSymbol* resultType)
        {
            auto state = std::make_shared<AsyncScopeState>();
            state->domain = &ctx.Domain;
            state->collector = &ctx.Collector;

            if (resultType == nullptr)
            {
                state->task = ctx.Collector.AllocateInstance(CLASS_TASK);
                state->isValueTask = false;
                SetTaskState(state->task, CLASS_TASK_StateField, AsyncState::PENDING, ctx.Collector);
            }
            else
            {
                state->task = ctx.Collector.AllocateGeneric(CLASS_VALUETASK, { resultType });
                state->isValueTask = true;
                SetTaskState(state->task, CLASS_VALUETASK_StateField, AsyncState::PENDING, ctx.Collector);
            }

            state->task->IsTaskLike = true;
            state->task->AsyncNativeState = state.get();
            ctx.Domain.GetEventLoop().RootTask(state->task);
            return state;
        }
    }

    AsyncScope::AsyncScope(std::shared_ptr<detail::AsyncScopeState> state)
        : m_state(std::move(state)) { }

    bool AsyncScope::IsValid() const noexcept
    {
        return m_state != nullptr;
    }

    ObjectInstance* AsyncScope::TaskObject() const noexcept
    {
        return m_state ? m_state->task : nullptr;
    }

    ApplicationDomain& AsyncScope::Domain() const noexcept
    {
        return *m_state->domain;
    }

    GarbageCollector& AsyncScope::Collector() const noexcept
    {
        return *m_state->collector;
    }

    std::shared_ptr<detail::AsyncScopeState> AsyncScope::ShareState() const noexcept
    {
        return m_state;
    }

    void AsyncScope::Complete()
    {
        if (!m_state || m_state->completed)
            return;

        if (m_state->isValueTask)
            m_state->SetValueTaskResult(GarbageCollector::NullInstance);
        else
            m_state->CompleteTask();
    }

    void AsyncScope::Fail(ObjectInstance* exception)
    {
        if (!m_state || m_state->completed)
            return;

        if (exception == nullptr)
            exception = GarbageCollector::NullInstance;

        if (m_state->isValueTask)
            m_state->FailValueTask(exception);
        else
            m_state->FailTask(exception);
    }

    void AsyncScope::Fail(const std::wstring& message)
    {
        Fail(CreateRuntimeException(*m_state->collector, message));
    }

    void AsyncScope::Delay(std::int64_t milliseconds, std::function<void()> onComplete)
    {
        if (!m_state)
            return;

        struct DelayData
        {
            std::shared_ptr<detail::AsyncScopeState> scope;
            std::function<void()> callback;
            uv_timer_t* timer;
        };

        auto* timer = new uv_timer_t;
        uv_timer_init(m_state->domain->GetEventLoop().GetLoop(), timer);
        m_state->ActiveHandle = reinterpret_cast<uv_handle_t*>(timer);

        auto* data = new DelayData{ m_state, std::move(onComplete), timer };
        timer->data = data;

        uv_timer_start(timer, [](uv_timer_t* handle)
        {
            DelayData* data = static_cast<DelayData*>(handle->data);
            data->scope->ActiveHandle = nullptr;
            data->callback();

            uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* closed)
            {
                delete reinterpret_cast<uv_timer_t*>(closed);
            });

            delete data;
        }, static_cast<std::uint64_t>(milliseconds), 0);
    }

    void AsyncScope::RunOnThreadPool(std::function<void()> work, std::function<void()> onComplete)
    {
        if (!m_state)
            return;

        struct ThreadPoolData
        {
            std::shared_ptr<detail::AsyncScopeState> scope;
            std::function<void()> callback;
            uv_async_t* async;
        };

        auto* async = new uv_async_t;
        uv_async_init(m_state->domain->GetEventLoop().GetLoop(), async, [](uv_async_t* handle)
        {
            auto* data = static_cast<ThreadPoolData*>(handle->data);
            if (data->scope != nullptr)
                data->scope->ActiveHandle = nullptr;
            data->callback();

            uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* closed)
            {
                delete reinterpret_cast<uv_async_t*>(closed);
            });

            delete data;
        });

        m_state->ActiveHandle = reinterpret_cast<uv_handle_t*>(async);

        auto* data = new ThreadPoolData{ m_state, std::move(onComplete), async };
        async->data = data;

        std::thread([work, async, data]()
        {
            work();
            uv_async_send(async);
        }).detach();
    }

    namespace detail
    {
        static ObjectInstance* InvokeMethodWithResult(VirtualMachine& vm, MethodSymbol* method, std::initializer_list<ObjectInstance*> args)
        {
            std::vector<ObjectInstance*> argVec(args);
            return vm.InvokeMethod(method, argVec.data(), argVec.size());
        }

        static ObjectInstance* GetAwaiterObject(VirtualMachine& vm, ObjectInstance* awaitable)
        {
            if (awaitable == nullptr || awaitable == GarbageCollector::NullInstance)
                return GarbageCollector::NullInstance;

            TypeSymbol* type = const_cast<TypeSymbol*>(awaitable->getInfo());
            MethodSymbol* getAwaiter = type->FindInterfaceImplementation(TRAIT_AWAITABLE_GetAwaiter);

            if (getAwaiter != nullptr)
                return InvokeMethodWithResult(vm, getAwaiter, { awaitable });

            return awaitable;
        }

        static bool InvokeIsCompleted(VirtualMachine& vm, ObjectInstance* awaiter)
        {
            TypeSymbol* type = const_cast<TypeSymbol*>(awaiter->getInfo());
            MethodSymbol* impl = type->FindInterfaceImplementation(TRAIT_AWAITER_IsCompleted);

            if (impl == nullptr)
                return true;

            ObjectInstance* result = InvokeMethodWithResult(vm, impl, { awaiter });
            if (result == nullptr || result == GarbageCollector::NullInstance)
                return false;

            return result->AsBoolean();
        }

        static void InvokeOnCompleted(VirtualMachine& vm, ObjectInstance* awaiter, ObjectInstance* continuation)
        {
            TypeSymbol* type = const_cast<TypeSymbol*>(awaiter->getInfo());
            MethodSymbol* impl = type->FindInterfaceImplementation(TRAIT_AWAITER_OnCompleted);

            if (impl == nullptr)
                return;

            InvokeMethodWithResult(vm, impl, { continuation, awaiter });
        }

        static ObjectInstance* InvokeGetResult(VirtualMachine& vm, ObjectInstance* awaiter)
        {
            TypeSymbol* type = const_cast<TypeSymbol*>(awaiter->getInfo());
            MethodSymbol* impl = type->FindInterfaceImplementation(TRAIT_AWAITER_GetResult);

            if (impl == nullptr)
                return GarbageCollector::NullInstance;

            return InvokeMethodWithResult(vm, impl, { awaiter });
        }
    }

    void AsyncScope::Await(ObjectInstance* awaitable, std::function<void()> onComplete)
    {
        if (!m_state)
            return;

        VirtualMachine& vm = m_state->domain->GetVirtualMachine();
        ObjectInstance* awaiter = detail::GetAwaiterObject(vm, awaitable);
        if (awaiter == nullptr || awaiter == GarbageCollector::NullInstance)
        {
            onComplete();
            return;
        }

        if (detail::InvokeIsCompleted(vm, awaiter))
        {
            onComplete();
            return;
        }

        ObjectInstance* continuation = detail::CreateNativeContinuation(*m_state, std::move(onComplete));
        detail::InvokeOnCompleted(vm, awaiter, continuation);
    }

    void AsyncScope::AwaitResult(ObjectInstance* awaitable, std::function<void(ObjectInstance* result)> onComplete)
    {
        if (!m_state)
            return;

        VirtualMachine& vm = m_state->domain->GetVirtualMachine();
        ObjectInstance* awaiter = detail::GetAwaiterObject(vm, awaitable);
        if (awaiter == nullptr || awaiter == GarbageCollector::NullInstance)
        {
            onComplete(GarbageCollector::NullInstance);
            return;
        }

        auto invokeResult = [&vm, awaiter, onComplete]() mutable
        {
            ObjectInstance* result = detail::InvokeGetResult(vm, awaiter);
            onComplete(result);
        };

        if (detail::InvokeIsCompleted(vm, awaiter))
        {
            invokeResult();
            return;
        }

        ObjectInstance* continuation = detail::CreateNativeContinuation(*m_state, std::move(invokeResult));
        detail::InvokeOnCompleted(vm, awaiter, continuation);
    }

    ObjectInstance* DoAsync(const CallState& ctx, std::function<void(AsyncScope)> work)
    {
        auto state = detail::CreateAsyncScopeState(ctx, nullptr);
        ObjectInstance* task = state->task;

        AsyncScope scope(std::move(state));
        work(scope);

        return task;
    }

    ObjectInstance* CompletedTask(const CallState& ctx)
    {
        ObjectInstance* task = ctx.Collector.AllocateInstance(CLASS_TASK);
        task->IsTaskLike = true;

        SetTaskState(task, CLASS_TASK_StateField, AsyncState::COMPLETED, ctx.Collector);
        return task;
    }

    ObjectInstance* FaultedTask(const CallState& ctx, const std::wstring& message)
    {
        ObjectInstance* task = ctx.Collector.AllocateInstance(CLASS_TASK);
        task->IsTaskLike = true;

        SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, ctx.Collector);
        task->SetField(CLASS_TASK_ExceptionField->SlotIndex, CreateRuntimeException(ctx.Collector, message));
        return task;
    }

    ObjectInstance* FaultedTask(const CallState& ctx, ObjectInstance* exception)
    {
        ObjectInstance* task = ctx.Collector.AllocateInstance(CLASS_TASK);
        task->IsTaskLike = true;

        SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, ctx.Collector);
        task->SetField(CLASS_TASK_ExceptionField->SlotIndex, exception != nullptr ? exception : GarbageCollector::NullInstance);
        return task;
    }

    ObjectInstance* CreateRuntimeException(GarbageCollector& collector, const std::wstring& message)
    {
        ObjectInstance* ex = collector.AllocateInstance(SymbolTable::StandardTypes::RuntimeException);
        ex->SetField(SymbolTable::StandardTypes::RuntimeExceptionMessageField->SlotIndex, collector.FromValue(message));
        ex->SetField(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField->SlotIndex, collector.FromValue(std::wstring()));
        return ex;
    }

    namespace detail
    {
        ObjectInstance* CreateNativeContinuation(AsyncScopeState& state, std::function<void()> callback)
        {
            ObjectInstance* continuation = state.collector->AllocateInstance(SymbolTable::StandardTypes::NativeContinuation);
            g_nativeContinuationCallbacks[continuation] = std::move(callback);
            return continuation;
        }

        void InvokeNativeContinuationCallback(ObjectInstance* continuation)
        {
            auto it = g_nativeContinuationCallbacks.find(continuation);
            if (it == g_nativeContinuationCallbacks.end())
                return;

            auto callback = std::move(it->second);
            g_nativeContinuationCallbacks.erase(it);

            if (callback)
                callback();
        }
    }
}
