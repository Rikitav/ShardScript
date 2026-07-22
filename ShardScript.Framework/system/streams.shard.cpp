#include <ShardScript.hpp>
#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/NativeAsync.hpp> 

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace shard;

// ============================================================================
// Cached stream symbols
// ============================================================================

static TypeSymbol* g_CancellationToken = nullptr;
static FieldSymbol* g_CancellationToken_SourceField = nullptr;
static FieldSymbol* g_CancellationTokenSource_CanceledField = nullptr;

static InterfaceSymbol* g_IStream = nullptr;
static InterfaceSymbol* g_IReadableStream = nullptr;
static InterfaceSymbol* g_IWritableStream = nullptr;

static EnumSymbol* g_SeekOrigin = nullptr;

// ============================================================================
// MemoryStream field symbols
// ============================================================================

static ClassSymbol* g_MemoryStream = nullptr;
static FieldSymbol* g_MemoryStream_Buffer = nullptr;
static FieldSymbol* g_MemoryStream_Position = nullptr;
static FieldSymbol* g_MemoryStream_Length = nullptr;
static FieldSymbol* g_MemoryStream_Capacity = nullptr;
static FieldSymbol* g_MemoryStream_Writable = nullptr;
static FieldSymbol* g_MemoryStream_IsOpen = nullptr;

// ============================================================================
// Helpers
// ============================================================================

static bool IsCancellationRequested(ObjectInstance* token)
{
    if (token == nullptr || token == GarbageCollector::NullInstance)
        return false;

    ObjectInstance* source = token->GetField(g_CancellationToken_SourceField->SlotIndex);
    if (source == nullptr || source == GarbageCollector::NullInstance)
        return false;

    ObjectInstance* canceled = source->GetField(g_CancellationTokenSource_CanceledField->SlotIndex);
    if (canceled == nullptr || canceled == GarbageCollector::NullInstance)
        return false;

    return canceled->AsInteger() != 0;
}

static void EnsureTokenSymbols(SymbolTable* table)
{
    if (g_CancellationToken != nullptr)
        return;

    g_CancellationToken = SemanticModel::FindTypeByName(table, L"async.CancellationToken");
    if (g_CancellationToken != nullptr)
    {
        g_CancellationToken_SourceField = SemanticModel::FindFieldByName(g_CancellationToken, L"_source");
    }

    TypeSymbol* sourceType = SemanticModel::FindTypeByName(table, L"async.CancellationTokenSource");
    if (sourceType != nullptr)
    {
        g_CancellationTokenSource_CanceledField = SemanticModel::FindFieldByName(sourceType, L"_canceled");
    }
}

static ObjectInstance* CanceledTask(const CallState& context)
{
    ObjectInstance* task = context.Collector.AllocateInstance(CLASS_TASK);
    task->IsTaskLike = true;
    SetTaskState(task, CLASS_TASK_StateField, AsyncState::FAULTED, context.Collector);
    task->SetField(CLASS_TASK_ExceptionField->SlotIndex, shard::CreateRuntimeException(context.Collector, L"Operation canceled."));
    return task;
}

static ObjectInstance* CanceledValueTaskInt(const CallState& context)
{
    ObjectInstance* task = context.Collector.AllocateGeneric(CLASS_VALUETASK, std::vector<TypeSymbol*>{ TYPE_INT });
    task->IsTaskLike = true;
    
    SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, context.Collector);
    task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, shard::CreateRuntimeException(context.Collector, L"Operation canceled."));
    return task;
}

static void EnsureOpen(ObjectInstance* instance)
{
    ObjectInstance* isOpen = instance->GetField(g_MemoryStream_IsOpen->SlotIndex);
    if (isOpen == nullptr || !isOpen->AsBoolean())
        throw std::runtime_error("Cannot access a closed stream.");
}

static void EnsureWritable(ObjectInstance* instance)
{
    ObjectInstance* writable = instance->GetField(g_MemoryStream_Writable->SlotIndex);
    if (writable == nullptr || !writable->AsBoolean())
        throw std::runtime_error("Cannot write to a non-writable stream.");
}

static std::int64_t GetPosition(ObjectInstance* instance)
{
    return instance->GetField(g_MemoryStream_Position->SlotIndex)->AsInteger();
}

static void SetPosition(ObjectInstance* instance, std::int64_t value, GarbageCollector& gc)
{
    instance->SetField(g_MemoryStream_Position->SlotIndex, gc.FromValue(value));
}

static std::int64_t GetLength(ObjectInstance* instance)
{
    return instance->GetField(g_MemoryStream_Length->SlotIndex)->AsInteger();
}

static void SetLength(ObjectInstance* instance, std::int64_t value, GarbageCollector& gc)
{
    instance->SetField(g_MemoryStream_Length->SlotIndex, gc.FromValue(value));
}

static std::int64_t GetCapacity(ObjectInstance* instance)
{
    return instance->GetField(g_MemoryStream_Capacity->SlotIndex)->AsInteger();
}

static void SetCapacity(ObjectInstance* instance, std::int64_t value, GarbageCollector& gc)
{
    instance->SetField(g_MemoryStream_Capacity->SlotIndex, gc.FromValue(value));
}

static ObjectInstance* GetBuffer(ObjectInstance* instance)
{
    return instance->GetField(g_MemoryStream_Buffer->SlotIndex);
}

static void SetBuffer(ObjectInstance* instance, ObjectInstance* buffer)
{
    instance->SetField(g_MemoryStream_Buffer->SlotIndex, buffer);
}

static void EnsureCapacity(ObjectInstance* instance, std::int64_t required, GarbageCollector& gc)
{
    std::int64_t capacity = GetCapacity(instance);
    if (required <= capacity)
        return;

    std::int64_t newCapacity = (std::max)(required, capacity * 2);
    if (newCapacity < 256)
        newCapacity = 256;

    ObjectInstance* oldBuffer = GetBuffer(instance);
    ObjectInstance* newBuffer = gc.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(newCapacity));

    std::int64_t length = GetLength(instance);
    for (std::int64_t i = 0; i < length; ++i)
    {
        ObjectInstance* element = oldBuffer->GetElement(static_cast<std::size_t>(i));
        newBuffer->SetElement(static_cast<std::size_t>(i), element);
    }

    SetBuffer(instance, newBuffer);
    SetCapacity(instance, newCapacity, gc);
}

// ============================================================================
// MemoryStream constructors
// ============================================================================

static ObjectInstance* shard_memoryStream_InitDefault(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];

    ObjectInstance* buffer = context.Collector.AllocateArray(TYPE_BYTE, 0);
    SetBuffer(instance, buffer);
    SetPosition(instance, 0, context.Collector);
    SetLength(instance, 0, context.Collector);
    SetCapacity(instance, 0, context.Collector);
    instance->SetField(g_MemoryStream_Writable->SlotIndex, context.Collector.FromValue(true));
    instance->SetField(g_MemoryStream_IsOpen->SlotIndex, context.Collector.FromValue(true));

    return instance;
}

static ObjectInstance* shard_memoryStream_InitWithBuffer(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];

    SetBuffer(instance, buffer);
    SetPosition(instance, 0, context.Collector);
    std::int64_t length = static_cast<std::int64_t>(buffer->GetArrayLength());
    SetLength(instance, length, context.Collector);
    SetCapacity(instance, length, context.Collector);
    instance->SetField(g_MemoryStream_Writable->SlotIndex, context.Collector.FromValue(false));
    instance->SetField(g_MemoryStream_IsOpen->SlotIndex, context.Collector.FromValue(true));

    return instance;
}

static ObjectInstance* shard_memoryStream_InitWithCapacity(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t capacity = context.Args[1]->AsInteger();
    if (capacity < 0)
        throw std::runtime_error("Capacity cannot be negative.");

    ObjectInstance* buffer = context.Collector.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(capacity));
    SetBuffer(instance, buffer);
    SetPosition(instance, 0, context.Collector);
    SetLength(instance, 0, context.Collector);
    SetCapacity(instance, capacity, context.Collector);
    instance->SetField(g_MemoryStream_Writable->SlotIndex, context.Collector.FromValue(true));
    instance->SetField(g_MemoryStream_IsOpen->SlotIndex, context.Collector.FromValue(true));

    return instance;
}

// ============================================================================
// MemoryStream methods
// ============================================================================

static ObjectInstance* shard_memoryStream_Read(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];

    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    EnsureOpen(instance);

    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    std::int64_t position = GetPosition(instance);
    std::int64_t length = GetLength(instance);
    std::int64_t available = length - position;
    if (available <= 0)
        return context.Collector.FromValue(static_cast<std::int64_t>(0));

    std::int64_t toRead = (std::min)(count, available);
    ObjectInstance* source = GetBuffer(instance);

    for (std::int64_t i = 0; i < toRead; ++i)
    {
        ObjectInstance* value = source->GetElement(static_cast<std::size_t>(position + i));
        buffer->SetElement(static_cast<std::size_t>(offset + i), value);
    }

    SetPosition(instance, position + toRead, context.Collector);
    return context.Collector.FromValue(toRead);
}

static ObjectInstance* shard_memoryStream_Write(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];

    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    EnsureOpen(instance);
    EnsureWritable(instance);

    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    std::int64_t position = GetPosition(instance);
    EnsureCapacity(instance, position + count, context.Collector);

    ObjectInstance* target = GetBuffer(instance);
    for (std::int64_t i = 0; i < count; ++i)
    {
        ObjectInstance* value = buffer->GetElement(static_cast<std::size_t>(offset + i));
        target->SetElement(static_cast<std::size_t>(position + i), value);
    }

    std::int64_t newPosition = position + count;
    SetPosition(instance, newPosition, context.Collector);
    if (newPosition > GetLength(instance))
        SetLength(instance, newPosition, context.Collector);

    return nullptr;
}

static ObjectInstance* shard_memoryStream_Flush(const CallState& context) noexcept(false)
{
    (void)context;
    return nullptr;
}

static ObjectInstance* shard_memoryStream_Seek(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t offset = context.Args[1]->AsInteger();
    std::int64_t origin = context.Args[2]->AsInteger();

    EnsureOpen(instance);

    std::int64_t newPosition = 0;
    if (origin == 0)
        newPosition = offset;
    else if (origin == 1)
        newPosition = GetPosition(instance) + offset;
    else if (origin == 2)
        newPosition = GetLength(instance) + offset;
    else
        throw std::runtime_error("Invalid seek origin.");

    if (newPosition < 0)
        throw std::runtime_error("Cannot seek before the beginning of the stream.");

    SetPosition(instance, newPosition, context.Collector);
    return context.Collector.FromValue(newPosition);
}

static ObjectInstance* shard_memoryStream_SetLength(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();

    EnsureOpen(instance);
    EnsureWritable(instance);

    if (value < 0)
        throw std::runtime_error("Length cannot be negative.");

    std::int64_t oldLength = GetLength(instance);
    EnsureCapacity(instance, value, context.Collector);

    ObjectInstance* buffer = GetBuffer(instance);
    for (std::int64_t i = oldLength; i < value; ++i)
    {
        buffer->SetElement(static_cast<std::size_t>(i), context.Collector.FromValue(static_cast<std::uint8_t>(0)));
    }

    SetLength(instance, value, context.Collector);
    if (GetPosition(instance) > value)
        SetPosition(instance, value, context.Collector);

    return nullptr;
}

static ObjectInstance* shard_memoryStream_ToArray(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureOpen(instance);

    std::int64_t length = GetLength(instance);
    ObjectInstance* result = context.Collector.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(length));
    ObjectInstance* buffer = GetBuffer(instance);

    for (std::int64_t i = 0; i < length; ++i)
    {
        ObjectInstance* value = buffer->GetElement(static_cast<std::size_t>(i));
        result->SetElement(static_cast<std::size_t>(i), value);
    }

    return result;
}

static ObjectInstance* shard_memoryStream_GetBuffer(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureOpen(instance);
    return GetBuffer(instance);
}

static ObjectInstance* shard_memoryStream_Close(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    instance->SetField(g_MemoryStream_IsOpen->SlotIndex, context.Collector.FromValue(false));
    return nullptr;
}

static ObjectInstance* shard_memoryStream_Dispose(const CallState& context) noexcept(false)
{
    return shard_memoryStream_Close(context);
}

// ============================================================================
// MemoryStream properties
// ============================================================================

static ObjectInstance* shard_memoryStream_Position_get(const CallState& context) noexcept(false)
{
    return context.Collector.FromValue(GetPosition(context.Args[0]));
}

static ObjectInstance* shard_memoryStream_Position_set(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();
    if (value < 0)
        throw std::runtime_error("Position cannot be negative.");

    EnsureOpen(instance);
    SetPosition(instance, value, context.Collector);
    return nullptr;
}

static ObjectInstance* shard_memoryStream_Length_get(const CallState& context) noexcept(false)
{
    return context.Collector.FromValue(GetLength(context.Args[0]));
}

static ObjectInstance* shard_memoryStream_Length_set(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();

    EnsureOpen(instance);
    EnsureWritable(instance);

    if (value < 0)
        throw std::runtime_error("Length cannot be negative.");

    std::int64_t oldLength = GetLength(instance);
    EnsureCapacity(instance, value, context.Collector);

    ObjectInstance* buffer = GetBuffer(instance);
    for (std::int64_t i = oldLength; i < value; ++i)
    {
        buffer->SetElement(static_cast<std::size_t>(i), context.Collector.FromValue(static_cast<std::uint8_t>(0)));
    }

    SetLength(instance, value, context.Collector);
    if (GetPosition(instance) > value)
        SetPosition(instance, value, context.Collector);

    return nullptr;
}

static ObjectInstance* shard_memoryStream_Capacity_get(const CallState& context) noexcept(false)
{
    return context.Collector.FromValue(GetCapacity(context.Args[0]));
}

static ObjectInstance* shard_memoryStream_Capacity_set(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();

    EnsureOpen(instance);
    EnsureWritable(instance);

    if (value < GetLength(instance))
        throw std::runtime_error("Capacity cannot be less than length.");

    if (value == GetCapacity(instance))
        return nullptr;

    ObjectInstance* oldBuffer = GetBuffer(instance);
    ObjectInstance* newBuffer = context.Collector.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(value));
    std::int64_t length = GetLength(instance);
    for (std::int64_t i = 0; i < length; ++i)
    {
        ObjectInstance* element = oldBuffer->GetElement(static_cast<std::size_t>(i));
        newBuffer->SetElement(static_cast<std::size_t>(i), element);
    }

    SetBuffer(instance, newBuffer);
    SetCapacity(instance, value, context.Collector);
    return nullptr;
}

// ============================================================================
// MemoryStream async methods
// ============================================================================

static ObjectInstance* shard_memoryStream_ReadAsync(const CallState& context) noexcept(false)
{
    std::int64_t read = shard_memoryStream_Read(context)->AsInteger();
    return CompletedValueTask(context, read);
}

static ObjectInstance* shard_memoryStream_ReadAsync_Cancel(const CallState& context) noexcept(false)
{
    if (IsCancellationRequested(context.Args[4]))
        return CanceledValueTaskInt(context);

    return shard_memoryStream_ReadAsync(context);
}

static ObjectInstance* shard_memoryStream_WriteAsync(const CallState& context) noexcept(false)
{
    shard_memoryStream_Write(context);
    return CompletedTask(context);
}

static ObjectInstance* shard_memoryStream_WriteAsync_Cancel(const CallState& context) noexcept(false)
{
    if (IsCancellationRequested(context.Args[4]))
        return CanceledTask(context);

    return shard_memoryStream_WriteAsync(context);
}

static ObjectInstance* shard_memoryStream_FlushAsync(const CallState& context) noexcept(false)
{
    shard_memoryStream_Flush(context);
    return CompletedTask(context);
}

static ObjectInstance* shard_memoryStream_FlushAsync_Cancel(const CallState& context) noexcept(false)
{
    if (IsCancellationRequested(context.Args[1]))
        return CanceledTask(context);

    return shard_memoryStream_FlushAsync(context);
}

// ============================================================================
// Library metadata
// ============================================================================

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.streams";
    lib.Description = L"Stream abstractions and MemoryStream";
    lib.Version = L"0.1.0";

    static const shard::ShardLibDependencyInfo deps[] =
    {
        { L"shard.async", L"1.0.0" }
    };

    lib.Dependencies = deps;
    lib.DependenciesLength = sizeof(deps) / sizeof(deps[0]);
}

// ============================================================================
// Library entry point
// ============================================================================

SHARDLIB_ENTRYPOINT
{
    EnsureTokenSymbols(context.GetSemanticModel().Table.get());

    SymbolFactory factory(context.GetSemanticModel().Table.get());
    TypeSymbol* byteArrayType = factory.Array(TYPE_BYTE);

    GenericTypeSymbol* valueTaskOfInt = factory.GenericType(CLASS_VALUETASK, { { L"T", TYPE_INT } });

    SymbolBuilder<NamespaceSymbol> ioNamespace(context, L"io");

    // ------------------------------------------------------------------------
    // enum SeekOrigin
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<EnumSymbol> seekOriginEnum = ioNamespace.AddEnum(L"SeekOrigin", false, ACS_PUBLIC);
        g_SeekOrigin = seekOriginEnum;

        seekOriginEnum
            .AddValue(L"Begin", 0)
            .AddValue(L"Current", 1)
            .AddValue(L"End", 2);
    }

    // ------------------------------------------------------------------------
    // interface IStream : IDisposable
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<InterfaceSymbol> streamInterface = ioNamespace.AddInterface(L"IStream");
        g_IStream = streamInterface;
        streamInterface.Implements(TRAIT_DISPOSABLE);

        SymbolBuilder<MethodSymbol> disposeMethod = streamInterface.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE);
        disposeMethod.Get()->IsAbstract = true;

        streamInterface.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // interface IReadableStream : IStream
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<InterfaceSymbol> readableInterface = ioNamespace.AddInterface(L"IReadableStream");
        g_IReadableStream = readableInterface;
        readableInterface.Implements(g_IStream);

        SymbolBuilder<MethodSymbol> readMethod = readableInterface.AddMethod(L"Read", TYPE_INT, LINK_INSTANCE);
        readMethod.Get()->IsAbstract = true;
        readMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT);

        SymbolBuilder<MethodSymbol> readAsyncMethod = readableInterface.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE);
        readAsyncMethod.Get()->IsAbstract = true;
        readAsyncMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT);

        SymbolBuilder<MethodSymbol> readAsyncCancelMethod = readableInterface.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE);
        readAsyncCancelMethod.Get()->IsAbstract = true;
        readAsyncCancelMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_CancellationToken);

        readableInterface.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // interface IWritableStream : IStream
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<InterfaceSymbol> writableInterface = ioNamespace.AddInterface(L"IWritableStream");
        g_IWritableStream = writableInterface;
        writableInterface.Implements(g_IStream);

        SymbolBuilder<MethodSymbol> writeMethod = writableInterface.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE);
        writeMethod.Get()->IsAbstract = true;
        writeMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT);

        SymbolBuilder<MethodSymbol> writeAsyncMethod = writableInterface.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE);
        writeAsyncMethod.Get()->IsAbstract = true;
        writeAsyncMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT);

        SymbolBuilder<MethodSymbol> writeAsyncCancelMethod = writableInterface.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE);
        writeAsyncCancelMethod.Get()->IsAbstract = true;
        writeAsyncCancelMethod
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_CancellationToken);

        SymbolBuilder<MethodSymbol> flushMethod = writableInterface.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE);
        flushMethod.Get()->IsAbstract = true;

        SymbolBuilder<MethodSymbol> flushAsyncMethod = writableInterface.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE);
        flushAsyncMethod.Get()->IsAbstract = true;

        SymbolBuilder<MethodSymbol> flushAsyncCancelMethod = writableInterface.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE);
        flushAsyncCancelMethod.Get()->IsAbstract = true;
        flushAsyncCancelMethod.AddParameter(L"cancellationToken", g_CancellationToken);

        writableInterface.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // class MemoryStream
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> memoryStreamClass = ioNamespace.AddClass(L"MemoryStream");
        g_MemoryStream = memoryStreamClass;
        memoryStreamClass.Implements(g_IReadableStream);
        memoryStreamClass.Implements(g_IWritableStream);
        memoryStreamClass.Implements(TRAIT_DISPOSABLE);

        g_MemoryStream_Buffer = memoryStreamClass
            .AddField(L"_buffer", byteArrayType, LINK_INSTANCE, ACS_PRIVATE);

        g_MemoryStream_Position = memoryStreamClass
            .AddField(L"_position", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

        g_MemoryStream_Length = memoryStreamClass
            .AddField(L"_length", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

        g_MemoryStream_Capacity = memoryStreamClass
            .AddField(L"_capacity", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

        g_MemoryStream_Writable = memoryStreamClass
            .AddField(L"_writable", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        g_MemoryStream_IsOpen = memoryStreamClass
            .AddField(L"_isOpen", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        memoryStreamClass.AddInit()
            .SetCallback(&shard_memoryStream_InitDefault);

        memoryStreamClass.AddInit()
            .AddParameter(L"buffer", byteArrayType)
            .SetCallback(&shard_memoryStream_InitWithBuffer);

        memoryStreamClass.AddInit()
            .AddParameter(L"capacity", TYPE_INT)
            .SetCallback(&shard_memoryStream_InitWithCapacity);

        memoryStreamClass.AddMethod(L"Read", TYPE_INT, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_memoryStream_Read);

        memoryStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_memoryStream_ReadAsync);

        memoryStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_CancellationToken)
            .SetCallback(&shard_memoryStream_ReadAsync_Cancel);

        memoryStreamClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_memoryStream_Write);

        memoryStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_memoryStream_WriteAsync);

        memoryStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", byteArrayType)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_CancellationToken)
            .SetCallback(&shard_memoryStream_WriteAsync_Cancel);

        memoryStreamClass.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_Flush);

        memoryStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_FlushAsync);

        memoryStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"cancellationToken", g_CancellationToken)
            .SetCallback(&shard_memoryStream_FlushAsync_Cancel);

        memoryStreamClass.AddMethod(L"Seek", TYPE_INT, LINK_INSTANCE)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"origin", g_SeekOrigin)
            .SetCallback(&shard_memoryStream_Seek);

        memoryStreamClass.AddMethod(L"SetLength", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_INT)
            .SetCallback(&shard_memoryStream_SetLength);

        memoryStreamClass.AddMethod(L"ToArray", byteArrayType, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_ToArray);

        memoryStreamClass.AddMethod(L"GetBuffer", byteArrayType, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_GetBuffer);

        memoryStreamClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_Close);

        memoryStreamClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_memoryStream_Dispose);

        SymbolBuilder<PropertySymbol> positionProp = memoryStreamClass.AddProperty(L"Position", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
        positionProp.AddGetter()
            .SetCallback(&shard_memoryStream_Position_get);
        positionProp.AddSetter()
            .SetCallback(&shard_memoryStream_Position_set);

        SymbolBuilder<PropertySymbol> lengthProp = memoryStreamClass.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
        lengthProp.AddGetter()
            .SetCallback(&shard_memoryStream_Length_get);
        lengthProp.AddSetter()
            .SetCallback(&shard_memoryStream_Length_set);

        SymbolBuilder<PropertySymbol> capacityProp = memoryStreamClass.AddProperty(L"Capacity", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
        capacityProp.AddGetter()
            .SetCallback(&shard_memoryStream_Capacity_get);
        capacityProp.AddSetter()
            .SetCallback(&shard_memoryStream_Capacity_set);

        memoryStreamClass.DeclareGlobal();
    }
}
