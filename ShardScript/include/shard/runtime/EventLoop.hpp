#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/runtime/ObjectInstance.hpp>

#include <uv.h>

#include <vector>
#include <algorithm>

namespace shard
{
    // ------------------------------------------------------------------------
    // Async state constants
    // ------------------------------------------------------------------------
    enum class AsyncState : std::int64_t
    {
        PENDING,
        COMPLETED,
        FAULTED,
    };

    // ------------------------------------------------------------------------
    // Helper state passed through a libuv handle's data pointer.
    // ------------------------------------------------------------------------
    struct DelayState
    {
        ApplicationDomain* Domain;
        ObjectInstance* Task;
        FieldSymbol* StateField;
        FieldSymbol* ContinuationField;
        MethodSymbol* MoveNextMethod;
    };

    /// <summary>
    /// Single-threaded cooperative event loop backed by libuv.
    /// Owned by ApplicationDomain and shared by all VMs in that domain.
    /// </summary>
    class SHARD_API EventLoop
    {
        uv_loop_t m_loop;
        std::vector<ObjectInstance*> m_rootedTasks;

    public:
        EventLoop();
        ~EventLoop();

        EventLoop(const EventLoop&) = delete;
        EventLoop& operator=(const EventLoop&) = delete;

        EventLoop(EventLoop&&) = delete;
        EventLoop& operator=(EventLoop&&) = delete;

        [[nodiscard]] uv_loop_t* GetLoop() noexcept;

        /// <summary>
        /// Run the loop until there are no pending async operations.
        /// </summary>
        void Run();

        /// <summary>
        /// Run the loop for a single iteration.
        /// </summary>
        void RunOnce();

        /// <summary>
        /// Stop the loop as soon as possible.
        /// </summary>
        void Stop();

        [[nodiscard]] bool IsAlive() const;

        /// <summary>
        /// Keep a Task / state-machine object alive while it is suspended in libuv.
        /// </summary>
        void RootTask(ObjectInstance* task);

        /// <summary>
        /// Release the strong reference acquired by RootTask.
        /// </summary>
        void UnrootTask(ObjectInstance* task);

        /// <summary>
        /// Returns true when the loop has no pending work and all rooted tasks are complete.
        /// </summary>
        [[nodiscard]] bool IsEmptyOrAllTasksCompleted() const;
    };

    AsyncState GetTaskState(ObjectInstance* task, FieldSymbol* stateField);
    void SetTaskState(ObjectInstance* task, FieldSymbol* stateField, AsyncState state, GarbageCollector& gc);
    void ResumeContinuation(ObjectInstance* task, FieldSymbol* continuationField, MethodSymbol* moveNextMethod, ApplicationDomain& domain);
}
