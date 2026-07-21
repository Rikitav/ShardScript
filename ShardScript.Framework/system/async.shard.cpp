#include <ShardScript.hpp>

#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/NativeAsync.hpp>

using namespace shard;

namespace
{
    // ------------------------------------------------------------------------
    // Registered symbols
    // ------------------------------------------------------------------------
    TypeSymbol* shard_TaskCompletionSource = nullptr;
    TypeParameterSymbol* shard_TaskCompletionSource_T = nullptr;
    FieldSymbol* shard_TaskCcompletetionSource_TaskField = nullptr;

    TypeSymbol* shard_CancellationTokenSource = nullptr;
    FieldSymbol* shard_CancellationTokenSource_CanceledField = nullptr;
    FieldSymbol* shard_CancellationTokenSource_TokenField = nullptr;

    TypeSymbol* shard_CancellationToken = nullptr;
    FieldSymbol* shard_CancellationToken_SourceField = nullptr;

    // ------------------------------------------------------------------------
    // TaskCompletionSource<T>
    // ------------------------------------------------------------------------
    static ObjectInstance* shard_async_TaskCompletionSource_Init(const CallState& context) noexcept
    {
        ObjectInstance* instance = context.Args[0];

        TypeSymbol* tArg = SymbolTable::Primitives::Any;
        if (TypeShape* shape = instance->getShape(); shape != nullptr && !shape->GenericArguments.empty())
            tArg = shape->GenericArguments[0];

        ObjectInstance* task = context.Collector.AllocateGeneric(CLASS_VALUETASK, { tArg });
        SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::PENDING, context.Collector);
        instance->SetField(shard_TaskCcompletetionSource_TaskField->SlotIndex, task);

        return instance;
    }

    static ObjectInstance* shard_async_TaskCompletionSource_Task_get(const CallState& context) noexcept
    {
        return context.Args[0]->GetField(shard_TaskCcompletetionSource_TaskField->SlotIndex);
    }

    static ObjectInstance* shard_async_TaskCompletionSource_SetResult(const CallState& context) noexcept
    {
        ObjectInstance* instance = context.Args[0];
        ObjectInstance* result = context.Args[1];
        ObjectInstance* task = instance->GetField(shard_TaskCcompletetionSource_TaskField->SlotIndex);

        SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::COMPLETED, context.Collector);
        task->SetField(CLASS_VALUETASK_ResultField->SlotIndex, result);
        ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

        return nullptr;
    }

    static ObjectInstance* shard_async_TaskCompletionSource_SetException(const CallState& context) noexcept
    {
        ObjectInstance* instance = context.Args[0];
        ObjectInstance* exception = context.Args[1];
        ObjectInstance* task = instance->GetField(shard_TaskCcompletetionSource_TaskField->SlotIndex);

        SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, context.Collector);
        task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, exception);
        ResumeContinuation(task, CLASS_VALUETASK_ContinuationField, TRAIT_ASYNCSTATE_MoveNext, context.Domain);

        return nullptr;
    }

    // ------------------------------------------------------------------------
    // CancellationToken / CancellationTokenSource
    // ------------------------------------------------------------------------
    static ObjectInstance* shard_async_CancellationTokenSource_Init(const CallState& context) noexcept
    {
        ObjectInstance* source = context.Args[0];

        ObjectInstance* token = context.Collector.AllocateInstance(
            static_cast<ClassSymbol*>(shard_CancellationToken));

        token->SetField(shard_CancellationToken_SourceField->SlotIndex, source);
        source->SetField(shard_CancellationTokenSource_TokenField->SlotIndex, token);
        source->SetField(shard_CancellationTokenSource_CanceledField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)));

        return source;
    }

    static ObjectInstance* shard_async_CancellationTokenSource_Cancel(const CallState& context) noexcept
    {
        ObjectInstance* source = context.Args[0];
        source->SetField(shard_CancellationTokenSource_CanceledField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(1)));
        return nullptr;
    }

    static ObjectInstance* shard_async_CancellationTokenSource_Token_get(const CallState& context) noexcept
    {
        ObjectInstance* source = context.Args[0];
        return source->GetField(shard_CancellationTokenSource_TokenField->SlotIndex);
    }

    static ObjectInstance* shard_async_CT_IsCancellationRequested_get(const CallState& context) noexcept
    {
        ObjectInstance* token = context.Args[0];
        ObjectInstance* source = token->GetField(shard_CancellationToken_SourceField->SlotIndex);
        if (source == nullptr || source == GarbageCollector::NullInstance)
            return context.Collector.FromValue(false);

        if (source->getInfo() != shard_CancellationTokenSource)
            return context.Collector.FromValue(false);

        ObjectInstance* canceledObj = source->GetField(shard_CancellationTokenSource_CanceledField->SlotIndex);
        if (canceledObj == nullptr || canceledObj == GarbageCollector::NullInstance)
            return context.Collector.FromValue(false);

        return context.Collector.FromValue(canceledObj->AsInteger() != 0);
    }

    static ObjectInstance* shard_async_CT_CanBeCanceled_get(const CallState& context) noexcept
    {
        ObjectInstance* token = context.Args[0];
        ObjectInstance* source = token->GetField(shard_CancellationToken_SourceField->SlotIndex);
        return context.Collector.FromValue(source != nullptr && source != GarbageCollector::NullInstance);
    }
}

// =============================================================================
// Library metadata
// =============================================================================

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.async";
    lib.Description = L"Async/await support primitives";
    lib.Version = L"1.0.0";
}

// ------------------------------------------------------------------------
// Internal native-async API regression helpers
// ------------------------------------------------------------------------
static ObjectInstance* shard_async_NativeAsyncTests_Delay(const CallState& context) noexcept
{
    std::int64_t ms = context.Args[0]->AsInteger();

    return shard::DoAsync(context, [ms](shard::AsyncScope& async)
    {
        async.Delay(ms, [async]() mutable
        {
            async.Complete();
        });
    });
}

static ObjectInstance* shard_async_NativeAsyncTests_ValueTask(const CallState& context) noexcept
{
    std::int64_t value = context.Args[0]->AsInteger();
    std::int64_t ms = context.Args[1]->AsInteger();

    return shard::DoValueTask<std::int64_t>(context, [value, ms](shard::AsyncValueScope<std::int64_t>& async)
    {
        async.Delay(ms, [async, value]() mutable
        {
            async.Complete(value);
        });
    });
}

static ObjectInstance* shard_async_NativeAsyncTests_Fault(const CallState& context) noexcept
{
    (void)context;
    return shard::FaultedTask(context, L"native async fault");
}

static ObjectInstance* shard_async_NativeAsyncTests_Await(const CallState& context) noexcept
{
    ObjectInstance* inner = context.Args[0];
    return shard::DoAsync(context, [inner](shard::AsyncScope& async)
    {
        async.Await(inner, [async]() mutable
        {
            async.Complete();
        });
    });
}

// =============================================================================
// Entry point
// =============================================================================

SHARDLIB_ENTRYPOINT
{
    SymbolFactory factory(context.GetSemanticModel().Table.get());
    SymbolBuilder<NamespaceSymbol> asyncNamespace(context, L"async");

    // -------------------------------------------------------------------------
    // class TaskCompletionSource<T>
    // -------------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> tcsClass = asyncNamespace.AddClass(L"TaskCompletionSource");
    shard_TaskCompletionSource = tcsClass;

    shard_TaskCompletionSource_T = tcsClass.AddTypeParameter(L"T");

    GenericTypeSymbol* valueTaskOfT = factory.GenericType(CLASS_VALUETASK, { { L"T", shard_TaskCompletionSource_T } });

    shard_TaskCcompletetionSource_TaskField = tcsClass
        .AddField(L"_task", valueTaskOfT, LINK_INSTANCE, ACS_PRIVATE);

    tcsClass.AddInit()
        .SetCallback(&shard_async_TaskCompletionSource_Init);

    tcsClass.AddProperty(L"Task", valueTaskOfT, LINK_INSTANCE)
        .AddGetter()
        .SetCallback(&shard_async_TaskCompletionSource_Task_get);

    tcsClass.AddMethod(L"SetResult", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"result", shard_TaskCompletionSource_T)
        .SetCallback(&shard_async_TaskCompletionSource_SetResult);

    tcsClass.AddMethod(L"SetException", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"exception", TYPE_ANY)
        .SetCallback(&shard_async_TaskCompletionSource_SetException);

    tcsClass.DeclareGlobal();

    // -------------------------------------------------------------------------
    // class CancellationToken
    // -------------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> ctsClass = asyncNamespace.AddClass(L"CancellationTokenSource");
    SymbolBuilder<ClassSymbol> ctClass = asyncNamespace.AddClass(L"CancellationToken");

    shard_CancellationToken = ctClass;
    shard_CancellationTokenSource = ctsClass;

    shard_CancellationToken_SourceField = ctClass
        .AddField(L"_source", shard_CancellationTokenSource, LINK_INSTANCE, ACS_PRIVATE);

    ctClass.AddProperty(L"IsCancellationRequested", TYPE_BOOL, LINK_INSTANCE)
        .AddGetter()
        .SetCallback(&shard_async_CT_IsCancellationRequested_get);

    ctClass.AddProperty(L"CanBeCanceled", TYPE_BOOL, LINK_INSTANCE)
        .AddGetter()
        .SetCallback(&shard_async_CT_CanBeCanceled_get);

    ctClass.DeclareGlobal();

    // -------------------------------------------------------------------------
    // class CancellationTokenSource
    // -------------------------------------------------------------------------

    shard_CancellationTokenSource_CanceledField = ctsClass
        .AddField(L"_canceled", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    shard_CancellationTokenSource_TokenField = ctsClass
        .AddField(L"_token", shard_CancellationToken, LINK_INSTANCE, ACS_PRIVATE);

    ctsClass.AddInit()
        .SetCallback(&shard_async_CancellationTokenSource_Init);

    ctsClass.AddMethod(L"Cancel", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_async_CancellationTokenSource_Cancel);

    ctsClass.AddProperty(L"Token", shard_CancellationToken, LINK_INSTANCE)
        .AddGetter()
        .SetCallback(&shard_async_CancellationTokenSource_Token_get);

    ctsClass.DeclareGlobal();

    // -------------------------------------------------------------------------
    // class NativeAsyncTests (internal regression surface for NativeAsync API)
    // -------------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> testClass = asyncNamespace.AddClass(L"NativeAsyncTests");

    testClass.AddMethod(L"Delay", CLASS_TASK, LINK_STATIC)
        .AddParameter(L"milliseconds", TYPE_INT)
        .SetCallback(&shard_async_NativeAsyncTests_Delay);

    GenericTypeSymbol* valueTaskOfInt = factory.GenericType(CLASS_VALUETASK, { { L"T", TYPE_INT } });

    testClass.AddMethod(L"ValueTask", valueTaskOfInt, LINK_STATIC)
        .AddParameter(L"value", TYPE_INT)
        .AddParameter(L"milliseconds", TYPE_INT)
        .SetCallback(&shard_async_NativeAsyncTests_ValueTask);

    testClass.AddMethod(L"Fault", CLASS_TASK, LINK_STATIC)
        .SetCallback(&shard_async_NativeAsyncTests_Fault);

    testClass.AddMethod(L"Await", CLASS_TASK, LINK_STATIC)
        .AddParameter(L"task", CLASS_TASK)
        .SetCallback(&shard_async_NativeAsyncTests_Await);

    testClass.DeclareGlobal();
}
