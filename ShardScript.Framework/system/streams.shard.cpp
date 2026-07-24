#include <ShardScript.hpp>
#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/NativeAsync.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <utilities/Strings.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
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
// Reader/Writer field symbols
// ============================================================================

static TypeSymbol* g_ByteArrayType = nullptr;

static ClassSymbol* g_StreamReader = nullptr;
static FieldSymbol* g_StreamReader_Stream = nullptr;
static FieldSymbol* g_StreamReader_Disposed = nullptr;

static ClassSymbol* g_StreamWriter = nullptr;
static FieldSymbol* g_StreamWriter_Stream = nullptr;
static FieldSymbol* g_StreamWriter_Disposed = nullptr;

static ClassSymbol* g_BinaryReader = nullptr;
static FieldSymbol* g_BinaryReader_Stream = nullptr;
static FieldSymbol* g_BinaryReader_Disposed = nullptr;

static ClassSymbol* g_BinaryWriter = nullptr;
static FieldSymbol* g_BinaryWriter_Stream = nullptr;
static FieldSymbol* g_BinaryWriter_Disposed = nullptr;

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
// Generic stream method invocation helpers
// ============================================================================

static MethodSymbol* FindMethodBySignature(TypeSymbol* type, const std::wstring& name, const std::vector<TypeSymbol*>& paramTypes)
{
    if (type == nullptr)
        return nullptr;

    for (MethodSymbol* method : type->Methods)
    {
        if (method == nullptr || method->Name != name || method->Linking != LINK_INSTANCE)
            continue;
        if (method->Parameters.size() != paramTypes.size())
            continue;

        bool match = true;
        for (std::size_t i = 0; i < paramTypes.size(); ++i)
        {
            if (method->Parameters[i]->Type != paramTypes[i])
            {
                match = false;
                break;
            }
        }
        if (match)
            return method;
    }
    return nullptr;
}

static MethodSymbol* FindStreamMethod(ObjectInstance* stream, const std::wstring& name, const std::vector<TypeSymbol*>& paramTypes)
{
    return FindMethodBySignature(const_cast<TypeSymbol*>(stream->getInfo()), name, paramTypes);
}

static std::int64_t StreamReadRaw(const CallState& context, ObjectInstance* stream, ObjectInstance* buffer, std::int64_t offset, std::int64_t count)
{
    MethodSymbol* method = FindStreamMethod(stream, L"Read", { g_ByteArrayType, TYPE_INT, TYPE_INT });
    if (method == nullptr)
        throw std::runtime_error("Stream does not support Read.");

    ObjectRef s(stream);
    ObjectRef b(buffer);
    ObjectRef off(context.Collector.FromValue(offset));
    ObjectRef cnt(context.Collector.FromValue(count));
    ObjectInstance* args[] = { cnt, off, b, s };

    ObjectRef result(context.Runtimer.InvokeMethod(method, args, 4));
    return result->AsInteger();
}

static void StreamWriteRaw(const CallState& context, ObjectInstance* stream, ObjectInstance* buffer, std::int64_t offset, std::int64_t count)
{
    MethodSymbol* method = FindStreamMethod(stream, L"Write", { g_ByteArrayType, TYPE_INT, TYPE_INT });
    if (method == nullptr)
        throw std::runtime_error("Stream does not support Write.");

    ObjectRef s(stream);
    ObjectRef b(buffer);
    ObjectRef off(context.Collector.FromValue(offset));
    ObjectRef cnt(context.Collector.FromValue(count));
    ObjectInstance* args[] = { cnt, off, b, s };

    context.Runtimer.InvokeMethod(method, args, 4);
}

static void StreamFlushRaw(const CallState& context, ObjectInstance* stream)
{
    MethodSymbol* method = FindStreamMethod(stream, L"Flush", {});
    if (method == nullptr)
        return;

    ObjectRef s(stream);
    ObjectInstance* args[] = { s };
    context.Runtimer.InvokeMethod(method, args, 1);
}

static void StreamDisposeRaw(const CallState& context, ObjectInstance* stream)
{
    MethodSymbol* method = FindStreamMethod(stream, L"Dispose", {});
    if (method == nullptr)
        return;

    ObjectRef s(stream);
    ObjectInstance* args[] = { s };
    context.Runtimer.InvokeMethod(method, args, 1);
}

static void ReadBytesExact(const CallState& context, ObjectInstance* stream, ObjectInstance* buffer, std::int64_t offset, std::int64_t count)
{
    if (count <= 0)
        return;

    std::int64_t totalRead = 0;
    while (totalRead < count)
    {
        std::int64_t read = StreamReadRaw(context, stream, buffer, offset + totalRead, count - totalRead);
        if (read <= 0)
            throw std::runtime_error("Unexpected end of stream.");
        totalRead += read;
    }
}

static void ReadBytesExact(const CallState& context, ObjectInstance* stream, ObjectInstance* buffer, std::int64_t count)
{
    ReadBytesExact(context, stream, buffer, 0, count);
}

static ObjectInstance* ReadBytesExact(const CallState& context, ObjectInstance* stream, std::int64_t count)
{
    if (count < 0)
        throw std::runtime_error("Count cannot be negative.");

    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(count)));
    ReadBytesExact(context, stream, buffer, count);
    return buffer.Instance;
}

static void WriteBytesRaw(const CallState& context, ObjectInstance* stream, const std::uint8_t* data, std::int64_t count)
{
    if (count <= 0)
        return;

    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, static_cast<std::size_t>(count)));
    for (std::int64_t i = 0; i < count; ++i)
        buffer->SetElement(static_cast<std::size_t>(i), context.Collector.FromValue(data[i]));

    StreamWriteRaw(context, stream, buffer, 0, count);
}

static void WriteInt32Raw(const CallState& context, ObjectInstance* stream, std::int32_t value)
{
    std::uint8_t bytes[4];
    bytes[0] = static_cast<std::uint8_t>(value & 0xFF);
    bytes[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    bytes[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
    WriteBytesRaw(context, stream, bytes, 4);
}

static void WriteInt64Raw(const CallState& context, ObjectInstance* stream, std::int64_t value)
{
    std::uint8_t bytes[8];
    for (int i = 0; i < 8; ++i)
        bytes[i] = static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF);
    WriteBytesRaw(context, stream, bytes, 8);
}

static void WriteDoubleRaw(const CallState& context, ObjectInstance* stream, double value)
{
    std::uint8_t bytes[8];
    std::memcpy(bytes, &value, sizeof(double));
    WriteBytesRaw(context, stream, bytes, 8);
}

static std::int32_t ReadInt32Raw(const CallState& context, ObjectInstance* stream)
{
    std::uint8_t bytes[4];
    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 4));
    ReadBytesExact(context, stream, buffer, 4);
    for (int i = 0; i < 4; ++i)
        bytes[i] = buffer->GetElement(static_cast<std::size_t>(i))->AsByte();

    return static_cast<std::int32_t>(bytes[0])
        | (static_cast<std::int32_t>(bytes[1]) << 8)
        | (static_cast<std::int32_t>(bytes[2]) << 16)
        | (static_cast<std::int32_t>(bytes[3]) << 24);
}

static std::int64_t ReadInt64Raw(const CallState& context, ObjectInstance* stream)
{
    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 8));
    ReadBytesExact(context, stream, buffer, 8);

    std::int64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::int64_t>(buffer->GetElement(static_cast<std::size_t>(i))->AsByte()) << (i * 8);
    return value;
}

static double ReadDoubleRaw(const CallState& context, ObjectInstance* stream)
{
    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 8));
    ReadBytesExact(context, stream, buffer, 8);

    std::uint8_t bytes[8];
    for (int i = 0; i < 8; ++i)
        bytes[i] = buffer->GetElement(static_cast<std::size_t>(i))->AsByte();

    double value = 0.0;
    std::memcpy(&value, bytes, sizeof(double));
    return value;
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
// StreamReader / StreamWriter / BinaryReader / BinaryWriter helpers
// ============================================================================

static ObjectInstance* GetReaderStream(ObjectInstance* instance)
{
    return instance->GetField(g_StreamReader_Stream->SlotIndex);
}

static ObjectInstance* GetWriterStream(ObjectInstance* instance)
{
    return instance->GetField(g_StreamWriter_Stream->SlotIndex);
}

static ObjectInstance* GetBinaryReaderStream(ObjectInstance* instance)
{
    return instance->GetField(g_BinaryReader_Stream->SlotIndex);
}

static ObjectInstance* GetBinaryWriterStream(ObjectInstance* instance)
{
    return instance->GetField(g_BinaryWriter_Stream->SlotIndex);
}

static void EnsureReaderNotDisposed(ObjectInstance* instance)
{
    ObjectInstance* disposed = instance->GetField(g_StreamReader_Disposed->SlotIndex);
    if (disposed != nullptr && disposed->AsBoolean())
        throw std::runtime_error("Cannot access a disposed StreamReader.");
}

static void EnsureWriterNotDisposed(ObjectInstance* instance)
{
    ObjectInstance* disposed = instance->GetField(g_StreamWriter_Disposed->SlotIndex);
    if (disposed != nullptr && disposed->AsBoolean())
        throw std::runtime_error("Cannot access a disposed StreamWriter.");
}

static void EnsureBinaryReaderNotDisposed(ObjectInstance* instance)
{
    ObjectInstance* disposed = instance->GetField(g_BinaryReader_Disposed->SlotIndex);
    if (disposed != nullptr && disposed->AsBoolean())
        throw std::runtime_error("Cannot access a disposed BinaryReader.");
}

static void EnsureBinaryWriterNotDisposed(ObjectInstance* instance)
{
    ObjectInstance* disposed = instance->GetField(g_BinaryWriter_Disposed->SlotIndex);
    if (disposed != nullptr && disposed->AsBoolean())
        throw std::runtime_error("Cannot access a disposed BinaryWriter.");
}

// ============================================================================
// StreamReader
// ============================================================================

static ObjectInstance* shard_streamReader_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* stream = context.Args[1];

    instance->SetField(g_StreamReader_Stream->SlotIndex, stream);
    instance->SetField(g_StreamReader_Disposed->SlotIndex, context.Collector.FromValue(false));
    return instance;
}

static ObjectInstance* shard_streamReader_ReadLine(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureReaderNotDisposed(instance);

    ObjectInstance* stream = GetReaderStream(instance);
    std::vector<std::uint8_t> accumulator;

    ObjectRef byteBuffer(context.Collector.AllocateArray(TYPE_BYTE, 1));
    while (true)
    {
        std::int64_t read = StreamReadRaw(context, stream, byteBuffer, 0, 1);
        if (read == 0)
            break;

        std::uint8_t byte = byteBuffer->GetElement(0)->AsByte();
        if (byte == '\n')
            break;

        accumulator.push_back(byte);
    }

    if (accumulator.empty())
        return context.Collector.FromValue(std::wstring());

    std::string utf8(reinterpret_cast<const char*>(accumulator.data()), accumulator.size());
    return context.Collector.FromValue(shard::strings::Utf8ToWide(utf8));
}

static ObjectInstance* shard_streamReader_ReadToEnd(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureReaderNotDisposed(instance);

    ObjectInstance* stream = GetReaderStream(instance);
    std::vector<std::uint8_t> accumulator;
    std::uint8_t chunk[256];

    while (true)
    {
        ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 256));
        std::int64_t read = StreamReadRaw(context, stream, buffer, 0, 256);
        if (read <= 0)
            break;

        for (std::int64_t i = 0; i < read; ++i)
            accumulator.push_back(buffer->GetElement(static_cast<std::size_t>(i))->AsByte());
    }

    if (accumulator.empty())
        return context.Collector.FromValue(std::wstring());

    std::string utf8(reinterpret_cast<const char*>(accumulator.data()), accumulator.size());
    return context.Collector.FromValue(shard::strings::Utf8ToWide(utf8));
}

static ObjectInstance* shard_streamReader_Read(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureReaderNotDisposed(instance);

    ObjectInstance* stream = GetReaderStream(instance);
    ObjectRef byteBuffer(context.Collector.AllocateArray(TYPE_BYTE, 4));

    std::int64_t firstRead = StreamReadRaw(context, stream, byteBuffer, 0, 1);
    if (firstRead == 0)
        return context.Collector.FromValue(static_cast<std::int64_t>(-1));

    std::uint8_t first = byteBuffer->GetElement(0)->AsByte();
    int extraBytes = 0;
    if ((first & 0x80) == 0)
        extraBytes = 0;
    else if ((first & 0xE0) == 0xC0)
        extraBytes = 1;
    else if ((first & 0xF0) == 0xE0)
        extraBytes = 2;
    else if ((first & 0xF8) == 0xF0)
        extraBytes = 3;
    else
        throw std::runtime_error("Invalid UTF-8 sequence.");

    if (extraBytes > 0)
        ReadBytesExact(context, stream, byteBuffer, 1, extraBytes);


    std::uint8_t bytes[4] = { first, 0, 0, 0 };
    for (int i = 0; i < extraBytes; ++i)
        bytes[i + 1] = byteBuffer->GetElement(static_cast<std::size_t>(i + 1))->AsByte();

    std::string utf8(reinterpret_cast<const char*>(bytes), extraBytes + 1);
    std::wstring wide = shard::strings::Utf8ToWide(utf8);
    if (wide.empty())
        return context.Collector.FromValue(static_cast<std::int64_t>(-1));

    return context.Collector.FromValue(static_cast<std::int64_t>(wide[0]));
}

static ObjectInstance* shard_streamReader_Close(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureReaderNotDisposed(instance);

    ObjectInstance* stream = GetReaderStream(instance);
    StreamDisposeRaw(context, stream);
    instance->SetField(g_StreamReader_Disposed->SlotIndex, context.Collector.FromValue(true));
    return nullptr;
}

static ObjectInstance* shard_streamReader_Dispose(const CallState& context) noexcept(false)
{
    return shard_streamReader_Close(context);
}

// ============================================================================
// StreamWriter
// ============================================================================

static ObjectInstance* shard_streamWriter_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* stream = context.Args[1];

    instance->SetField(g_StreamWriter_Stream->SlotIndex, stream);
    instance->SetField(g_StreamWriter_Disposed->SlotIndex, context.Collector.FromValue(false));
    return instance;
}

static ObjectInstance* shard_streamWriter_Write(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureWriterNotDisposed(instance);

    ObjectInstance* stream = GetWriterStream(instance);
    const wchar_t* str = context.Args[1]->AsString();
    if (str == nullptr)
        return nullptr;

    std::string utf8 = shard::strings::WideToUtf8(str);
    if (!utf8.empty())
        WriteBytesRaw(context, stream, reinterpret_cast<const std::uint8_t*>(utf8.data()), static_cast<std::int64_t>(utf8.size()));

    return nullptr;
}

static ObjectInstance* shard_streamWriter_WriteLine(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    shard_streamWriter_Write(context);

    ObjectInstance* stream = GetWriterStream(instance);
    static const std::uint8_t newline = '\n';
    WriteBytesRaw(context, stream, &newline, 1);
    return nullptr;
}

static ObjectInstance* shard_streamWriter_Flush(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureWriterNotDisposed(instance);

    StreamFlushRaw(context, GetWriterStream(instance));
    return nullptr;
}

static ObjectInstance* shard_streamWriter_Close(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureWriterNotDisposed(instance);

    ObjectInstance* stream = GetWriterStream(instance);
    StreamFlushRaw(context, stream);
    StreamDisposeRaw(context, stream);
    instance->SetField(g_StreamWriter_Disposed->SlotIndex, context.Collector.FromValue(true));
    return nullptr;
}

static ObjectInstance* shard_streamWriter_Dispose(const CallState& context) noexcept(false)
{
    return shard_streamWriter_Close(context);
}

// ============================================================================
// BinaryReader
// ============================================================================

static ObjectInstance* shard_binaryReader_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* stream = context.Args[1];

    instance->SetField(g_BinaryReader_Stream->SlotIndex, stream);
    instance->SetField(g_BinaryReader_Disposed->SlotIndex, context.Collector.FromValue(false));
    return instance;
}

static ObjectInstance* shard_binaryReader_ReadBoolean(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);

    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 1));
    ReadBytesExact(context, GetBinaryReaderStream(instance), buffer, 1);
    return context.Collector.FromValue(buffer->GetElement(0)->AsByte() != 0);
}

static ObjectInstance* shard_binaryReader_ReadByte(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);

    ObjectRef buffer(context.Collector.AllocateArray(TYPE_BYTE, 1));
    ReadBytesExact(context, GetBinaryReaderStream(instance), buffer, 1);
    return context.Collector.FromValue(buffer->GetElement(0)->AsByte());
}

static ObjectInstance* shard_binaryReader_ReadInt32(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);
    return context.Collector.FromValue(static_cast<std::int64_t>(ReadInt32Raw(context, GetBinaryReaderStream(instance))));
}

static ObjectInstance* shard_binaryReader_ReadInt64(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);
    return context.Collector.FromValue(ReadInt64Raw(context, GetBinaryReaderStream(instance)));
}

static ObjectInstance* shard_binaryReader_ReadDouble(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);
    return context.Collector.FromValue(ReadDoubleRaw(context, GetBinaryReaderStream(instance)));
}

static ObjectInstance* shard_binaryReader_ReadString(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);

    ObjectInstance* stream = GetBinaryReaderStream(instance);
    std::int32_t length = ReadInt32Raw(context, stream);
    if (length < 0)
        throw std::runtime_error("Invalid string length in binary stream.");
    if (length == 0)
        return context.Collector.FromValue(std::wstring());

    ObjectRef bytes(ReadBytesExact(context, stream, length));
    std::string utf8;
    utf8.reserve(length);
    for (std::int32_t i = 0; i < length; ++i)
        utf8.push_back(static_cast<char>(bytes->GetElement(static_cast<std::size_t>(i))->AsByte()));

    return context.Collector.FromValue(shard::strings::Utf8ToWide(utf8));
}

static ObjectInstance* shard_binaryReader_ReadBytes(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);

    std::int64_t count = context.Args[1]->AsInteger();
    return ReadBytesExact(context, GetBinaryReaderStream(instance), count);
}

static ObjectInstance* shard_binaryReader_Close(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryReaderNotDisposed(instance);

    ObjectInstance* stream = GetBinaryReaderStream(instance);
    StreamDisposeRaw(context, stream);
    instance->SetField(g_BinaryReader_Disposed->SlotIndex, context.Collector.FromValue(true));
    return nullptr;
}

static ObjectInstance* shard_binaryReader_Dispose(const CallState& context) noexcept(false)
{
    return shard_binaryReader_Close(context);
}

// ============================================================================
// BinaryWriter
// ============================================================================

static ObjectInstance* shard_binaryWriter_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* stream = context.Args[1];

    instance->SetField(g_BinaryWriter_Stream->SlotIndex, stream);
    instance->SetField(g_BinaryWriter_Disposed->SlotIndex, context.Collector.FromValue(false));
    return instance;
}

static ObjectInstance* shard_binaryWriter_WriteBoolean(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    bool value = context.Args[1]->AsBoolean();
    std::uint8_t byte = value ? 1 : 0;
    WriteBytesRaw(context, GetBinaryWriterStream(instance), &byte, 1);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteByte(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    std::uint8_t value = static_cast<std::uint8_t>(context.Args[1]->AsByte());
    WriteBytesRaw(context, GetBinaryWriterStream(instance), &value, 1);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteInt32(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    std::int32_t value = static_cast<std::int32_t>(context.Args[1]->AsInteger());
    WriteInt32Raw(context, GetBinaryWriterStream(instance), value);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteInt64(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    std::int64_t value = context.Args[1]->AsInteger();
    WriteInt64Raw(context, GetBinaryWriterStream(instance), value);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteDouble(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    double value = context.Args[1]->AsDouble();
    WriteDoubleRaw(context, GetBinaryWriterStream(instance), value);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteString(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    ObjectInstance* stream = GetBinaryWriterStream(instance);
    const wchar_t* str = context.Args[1]->AsString();
    if (str == nullptr)
    {
        WriteInt32Raw(context, stream, 0);
        return nullptr;
    }

    std::string utf8 = shard::strings::WideToUtf8(str);
    std::int32_t length = static_cast<std::int32_t>(utf8.size());
    WriteInt32Raw(context, stream, length);
    if (length > 0)
        WriteBytesRaw(context, stream, reinterpret_cast<const std::uint8_t*>(utf8.data()), length);

    return nullptr;
}

static ObjectInstance* shard_binaryWriter_WriteBytes(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    ObjectInstance* stream = GetBinaryWriterStream(instance);
    ObjectInstance* bytes = context.Args[1];
    std::int64_t length = static_cast<std::int64_t>(bytes->GetArrayLength());
    StreamWriteRaw(context, stream, bytes, 0, length);
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_Flush(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    StreamFlushRaw(context, GetBinaryWriterStream(instance));
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_Close(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    EnsureBinaryWriterNotDisposed(instance);

    ObjectInstance* stream = GetBinaryWriterStream(instance);
    StreamFlushRaw(context, stream);
    StreamDisposeRaw(context, stream);
    instance->SetField(g_BinaryWriter_Disposed->SlotIndex, context.Collector.FromValue(true));
    return nullptr;
}

static ObjectInstance* shard_binaryWriter_Dispose(const CallState& context) noexcept(false)
{
    return shard_binaryWriter_Close(context);
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
    g_ByteArrayType = byteArrayType;

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

    // ------------------------------------------------------------------------
    // class StreamReader
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> streamReaderClass = ioNamespace.AddClass(L"StreamReader");
        g_StreamReader = streamReaderClass;
        streamReaderClass.Implements(TRAIT_DISPOSABLE);

        g_StreamReader_Stream = streamReaderClass
            .AddField(L"_stream", g_IReadableStream, LINK_INSTANCE, ACS_PRIVATE);

        g_StreamReader_Disposed = streamReaderClass
            .AddField(L"_disposed", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        streamReaderClass.AddInit()
            .AddParameter(L"stream", g_IReadableStream)
            .SetCallback(&shard_streamReader_Init);

        streamReaderClass.AddMethod(L"ReadLine", TYPE_STRING, LINK_INSTANCE)
            .SetCallback(&shard_streamReader_ReadLine);

        streamReaderClass.AddMethod(L"ReadToEnd", TYPE_STRING, LINK_INSTANCE)
            .SetCallback(&shard_streamReader_ReadToEnd);

        streamReaderClass.AddMethod(L"Read", TYPE_INT, LINK_INSTANCE)
            .SetCallback(&shard_streamReader_Read);

        streamReaderClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_streamReader_Close);

        streamReaderClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_streamReader_Dispose);

        streamReaderClass.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // class StreamWriter
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> streamWriterClass = ioNamespace.AddClass(L"StreamWriter");
        g_StreamWriter = streamWriterClass;
        streamWriterClass.Implements(TRAIT_DISPOSABLE);

        g_StreamWriter_Stream = streamWriterClass
            .AddField(L"_stream", g_IWritableStream, LINK_INSTANCE, ACS_PRIVATE);

        g_StreamWriter_Disposed = streamWriterClass
            .AddField(L"_disposed", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        streamWriterClass.AddInit()
            .AddParameter(L"stream", g_IWritableStream)
            .SetCallback(&shard_streamWriter_Init);

        streamWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_STRING)
            .SetCallback(&shard_streamWriter_Write);

        streamWriterClass.AddMethod(L"WriteLine", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_STRING)
            .SetCallback(&shard_streamWriter_WriteLine);

        streamWriterClass.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_streamWriter_Flush);

        streamWriterClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_streamWriter_Close);

        streamWriterClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_streamWriter_Dispose);

        streamWriterClass.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // class BinaryReader
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> binaryReaderClass = ioNamespace.AddClass(L"BinaryReader");
        g_BinaryReader = binaryReaderClass;
        binaryReaderClass.Implements(TRAIT_DISPOSABLE);

        g_BinaryReader_Stream = binaryReaderClass
            .AddField(L"_stream", g_IReadableStream, LINK_INSTANCE, ACS_PRIVATE);

        g_BinaryReader_Disposed = binaryReaderClass
            .AddField(L"_disposed", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        binaryReaderClass.AddInit()
            .AddParameter(L"stream", g_IReadableStream)
            .SetCallback(&shard_binaryReader_Init);

        binaryReaderClass.AddMethod(L"ReadBoolean", TYPE_BOOL, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadBoolean);

        binaryReaderClass.AddMethod(L"ReadByte", TYPE_BYTE, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadByte);

        binaryReaderClass.AddMethod(L"ReadInt32", TYPE_INT, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadInt32);

        binaryReaderClass.AddMethod(L"ReadInt64", TYPE_INT, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadInt64);

        binaryReaderClass.AddMethod(L"ReadDouble", TYPE_DOUBLE, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadDouble);

        binaryReaderClass.AddMethod(L"ReadString", TYPE_STRING, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_ReadString);

        binaryReaderClass.AddMethod(L"ReadBytes", byteArrayType, LINK_INSTANCE)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_binaryReader_ReadBytes);

        binaryReaderClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_Close);

        binaryReaderClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_binaryReader_Dispose);

        binaryReaderClass.DeclareGlobal();
    }

    // ------------------------------------------------------------------------
    // class BinaryWriter
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> binaryWriterClass = ioNamespace.AddClass(L"BinaryWriter");
        g_BinaryWriter = binaryWriterClass;
        binaryWriterClass.Implements(TRAIT_DISPOSABLE);

        g_BinaryWriter_Stream = binaryWriterClass
            .AddField(L"_stream", g_IWritableStream, LINK_INSTANCE, ACS_PRIVATE);

        g_BinaryWriter_Disposed = binaryWriterClass
            .AddField(L"_disposed", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        binaryWriterClass.AddInit()
            .AddParameter(L"stream", g_IWritableStream)
            .SetCallback(&shard_binaryWriter_Init);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_BOOL)
            .SetCallback(&shard_binaryWriter_WriteBoolean);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_BYTE)
            .SetCallback(&shard_binaryWriter_WriteByte);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_INT)
            .SetCallback(&shard_binaryWriter_WriteInt64);

        binaryWriterClass.AddMethod(L"WriteInt32", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_INT)
            .SetCallback(&shard_binaryWriter_WriteInt32);

        binaryWriterClass.AddMethod(L"WriteInt64", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_INT)
            .SetCallback(&shard_binaryWriter_WriteInt64);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&shard_binaryWriter_WriteDouble);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_STRING)
            .SetCallback(&shard_binaryWriter_WriteString);

        binaryWriterClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", byteArrayType)
            .SetCallback(&shard_binaryWriter_WriteBytes);

        binaryWriterClass.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_binaryWriter_Flush);

        binaryWriterClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_binaryWriter_Close);

        binaryWriterClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_binaryWriter_Dispose);

        binaryWriterClass.DeclareGlobal();
    }
}
