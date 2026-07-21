#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <uv.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace shard
{
    struct CallState;
    class ApplicationDomain;
    class GarbageCollector;

    namespace detail
    {
        /// <summary>
        /// Internal heap state shared by an async operation and its callbacks.
        /// </summary>
        struct SHARD_API AsyncScopeState
        {
            ApplicationDomain* domain = nullptr;
            GarbageCollector* collector = nullptr;
            ObjectInstance* task = nullptr;
            uv_handle_t* ActiveHandle = nullptr;
            bool completed = false;
            bool isValueTask = false;

            void EnsureCompletedOnce();
            void CompleteTask();
            void FailTask(ObjectInstance* exception);
            void SetValueTaskResult(ObjectInstance* result);
            void FailValueTask(ObjectInstance* exception);
            void Halt();
        };

        SHARD_API std::shared_ptr<AsyncScopeState> CreateAsyncScopeState(const CallState& ctx, TypeSymbol* resultType);
        SHARD_API ObjectInstance* CreateNativeContinuation(AsyncScopeState& state, std::function<void()> callback);
        SHARD_API void InvokeNativeContinuationCallback(ObjectInstance* continuation);
    }

    /// <summary>
    /// Scope passed to the work lambda of DoAsync. Describes an async operation
    /// that eventually completes or faults a Task.
    /// </summary>
    class SHARD_API AsyncScope
    {
    public:
        AsyncScope() = default;
        explicit AsyncScope(std::shared_ptr<detail::AsyncScopeState> state);
        ~AsyncScope() = default;

        AsyncScope(const AsyncScope&) = default;
        AsyncScope& operator=(const AsyncScope&) = default;
        AsyncScope(AsyncScope&&) = default;
        AsyncScope& operator=(AsyncScope&&) = default;

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] ObjectInstance* TaskObject() const noexcept;

        [[nodiscard]] ApplicationDomain& Domain() const noexcept;
        [[nodiscard]] GarbageCollector& Collector() const noexcept;

        /// <summary>Mark the underlying Task as completed and resume awaiters.</summary>
        void Complete();

        /// <summary>Mark the underlying Task as faulted and resume awaiters.</summary>
        void Fail(ObjectInstance* exception);

        /// <summary>Mark the underlying Task as faulted with a RuntimeException.</summary>
        void Fail(const std::wstring& message);

        /// <summary>Schedule a callback after the given delay in milliseconds.</summary>
        void Delay(std::int64_t milliseconds, std::function<void()> onComplete);

        /// <summary>Run work on a thread-pool thread and call onComplete on the loop thread.</summary>
        void RunOnThreadPool(std::function<void()> work, std::function<void()> onComplete);

        /// <summary>Await an existing ShardScript awaitable and call onComplete when it completes.</summary>
        void Await(ObjectInstance* awaitable, std::function<void()> onComplete);

        /// <summary>Await an existing ShardScript awaitable and receive its boxed result.</summary>
        void AwaitResult(ObjectInstance* awaitable, std::function<void(ObjectInstance* result)> onComplete);

    protected:
        std::shared_ptr<detail::AsyncScopeState> m_state;
    };

    /// <summary>
    /// Scope passed to the work lambda of DoValueTask. Describes an async operation
    /// that produces a value.
    /// </summary>
    template <typename T>
    class AsyncValueScope : public AsyncScope
    {
    public:
        AsyncValueScope() = default;
        explicit AsyncValueScope(std::shared_ptr<detail::AsyncScopeState> state);

        /// <summary>Store the result and mark the underlying ValueTask as completed.</summary>
        void Complete(T value);
    };

    /// <summary>Start an async operation that returns async.Task.</summary>
    SHARD_API ObjectInstance* DoAsync(const CallState& ctx, std::function<void(AsyncScope&)> work);

    /// <summary>Start an async operation that returns async.ValueTask&lt;T&gt;.</summary>
    template <typename T>
    ObjectInstance* DoValueTask(const CallState& ctx, std::function<void(AsyncValueScope<T>&)> work);

    /// <summary>Return an already-completed async.Task.</summary>
    SHARD_API ObjectInstance* CompletedTask(const CallState& ctx);

    /// <summary>Return a faulted async.Task.</summary>
    SHARD_API ObjectInstance* FaultedTask(const CallState& ctx, const std::wstring& message);

    /// <summary>Return a faulted async.Task.</summary>
    SHARD_API ObjectInstance* FaultedTask(const CallState& ctx, ObjectInstance* exception);

    /// <summary>Return an already-completed async.ValueTask&lt;T&gt;.</summary>
    template <typename T>
    ObjectInstance* CompletedValueTask(const CallState& ctx, T value);

    /// <summary>Return a faulted async.ValueTask&lt;T&gt;.</summary>
    template <typename T>
    ObjectInstance* FaultedValueTask(const CallState& ctx, const std::wstring& message);

    /// <summary>Construct a RuntimeException instance with the given message.</summary>
    SHARD_API ObjectInstance* CreateRuntimeException(GarbageCollector& collector, const std::wstring& message);
}

#include <shard/runtime/NativeAsync.inl>
