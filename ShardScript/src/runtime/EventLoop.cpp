#include <shard/runtime/EventLoop.hpp>

#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <uv.h>

using namespace shard;

EventLoop::EventLoop()
{
    uv_loop_init(&m_loop);
}

EventLoop::~EventLoop()
{
    // Release any remaining rooted tasks before tearing down the loop.
    for (ObjectInstance* task : m_rootedTasks)
    {
        if (task != nullptr && task != GarbageCollector::NullInstance)
            task->DecrementReference();
    }

    m_rootedTasks.clear();
    uv_loop_close(&m_loop);
}

uv_loop_t* EventLoop::GetLoop() noexcept
{
    return &m_loop;
}

void EventLoop::Run()
{
    uv_run(&m_loop, UV_RUN_DEFAULT);
}

void EventLoop::RunOnce()
{
    uv_run(&m_loop, UV_RUN_ONCE);
}

void EventLoop::Stop()
{
    uv_stop(&m_loop);
}

bool EventLoop::IsAlive() const
{
    return uv_loop_alive(&m_loop) != 0;
}

void EventLoop::RootTask(ObjectInstance* task)
{
    if (task == nullptr || task == GarbageCollector::NullInstance)
        return;

    task->IncrementReference();
    m_rootedTasks.push_back(task);
}

void EventLoop::UnrootTask(ObjectInstance* task)
{
    if (task == nullptr || task == GarbageCollector::NullInstance)
        return;

    auto it = std::find(m_rootedTasks.begin(), m_rootedTasks.end(), task);
    if (it == m_rootedTasks.end())
        return;

    m_rootedTasks.erase(it);
    task->DecrementReference();
}

bool EventLoop::IsEmptyOrAllTasksCompleted() const
{
    if (!m_rootedTasks.empty())
        return false;

    return uv_loop_alive(&m_loop) == 0;
}

void shard::ResumeContinuation(ObjectInstance* task, FieldSymbol* continuationField, MethodSymbol* moveNextMethod, ApplicationDomain& domain)
{
    ObjectInstance* continuation = task->GetField(continuationField->SlotIndex);
    if (continuation == nullptr || continuation == GarbageCollector::NullInstance)
        return;

    if (moveNextMethod == nullptr)
        return;

    MethodSymbol* implementation = const_cast<TypeSymbol*>(continuation->getInfo())->FindInterfaceImplementation(moveNextMethod);
    if (implementation == nullptr)
        return;

    // Root the continuation before we clear it from the task's field.
    continuation->IncrementReference();
    task->SetField(continuationField->SlotIndex, GarbageCollector::NullInstance);

    // We are invoked from a libuv callback that has no proper VM calling frame.
    VirtualMachine& vm = domain.GetVirtualMachine();
    CallStackFrame* rootFrame = vm.PushFrame(implementation);
    rootFrame->PushStack(continuation);

    try
    {
        // The no-argument overload pops arguments from the previous frame into the new frame, then executes the method.
        vm.InvokeMethod(implementation);
    }
    catch (...)
    {
        // InvokeMethod does not clean up on exception. Pop any frames it may have left behind, then pop our synthetic root, and rethrow.
        while (vm.CurrentFrame() != rootFrame && vm.CurrentFrame() != nullptr)
            vm.PopFrame();

        vm.PopFrame();
        continuation->DecrementReference();
        throw;
    }

    vm.PopFrame(); // synthetic root
    continuation->DecrementReference();
}

AsyncState shard::GetTaskState(ObjectInstance* task, FieldSymbol* stateField)
{
    ObjectInstance* stateObj = task->GetField(stateField->SlotIndex);
    if (stateObj == nullptr || stateObj == GarbageCollector::NullInstance)
        return AsyncState::PENDING;

    return static_cast<AsyncState>(stateObj->AsInteger());
}

void shard::SetTaskState(ObjectInstance* task, FieldSymbol* stateField, AsyncState state, GarbageCollector& gc)
{
    task->SetField(stateField->SlotIndex, gc.FromValue(static_cast<std::int64_t>(state)));
}
