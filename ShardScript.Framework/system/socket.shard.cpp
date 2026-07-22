#include <ShardScript.hpp>
#include <shard/runtime/NativeAsync.hpp>

#include <string>
#include <cstdint>
#include <vector>
#include <memory>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <WinSock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    using socket_t = SOCKET;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define close_socket_native(s) closesocket(s)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <cstring>
    
    using socket_t = int;
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
    #define close_socket_native(s) ::close(s)
#endif

using namespace shard;

static bool InitNetwork() noexcept
{
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

ClassSymbol* shard_socket = nullptr;
FieldSymbol* shard_socket_handle = nullptr;
MethodSymbol* shard_socket_close = nullptr;

// Stream abstraction symbols (loaded from shard.streams at module load).
TypeSymbol* g_Stream_IStream = nullptr;
TypeSymbol* g_Stream_IReadableStream = nullptr;
TypeSymbol* g_Stream_IWritableStream = nullptr;

TypeSymbol* g_Stream_CancellationToken = nullptr;
FieldSymbol* g_Stream_CancellationToken_SourceField = nullptr;
FieldSymbol* g_Stream_CancellationTokenSource_CanceledField = nullptr;

// SocketStream state.
ClassSymbol* g_SocketStream = nullptr;
FieldSymbol* g_SocketStream_Handle = nullptr;
FieldSymbol* g_SocketStream_IsOpen = nullptr;

static ObjectInstance* shard_socket_init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (!InitNetwork())
        throw std::runtime_error("Failed to init inet.");

    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VAL)
        throw std::runtime_error("Failed to create socket handle.");

    instance->SetField(shard_socket_handle->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(sock)));
    return instance;
}

static ObjectInstance* shard_socket_Connect(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    const wchar_t* ip_w = context.Args[1]->AsString();
    int64_t port = context.Args[2]->AsInteger();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(false);

    std::string ip_narrow = strings::WideToUtf8(ip_w);
    sockaddr_in clientService{};
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, ip_narrow.data(), &clientService.sin_addr) <= 0)
        return context.Collector.FromValue(false);

    int result = connect(socket_handle,
        reinterpret_cast<sockaddr*>(&clientService),
        sizeof(clientService));

    return context.Collector.FromValue(result != SOCKET_ERROR_VAL);
}

static ObjectInstance* shard_socket_Send(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    const wchar_t* data_w = context.Args[1]->AsString();
    int data_length = static_cast<int>(context.Args[1]->AsStringLength());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(static_cast<int64_t>(-1));

    int bytesSent = send(socket_handle,
        reinterpret_cast<const char*>(data_w), data_length * sizeof(wchar_t), 0);

    return context.Collector.FromValue(static_cast<int64_t>(bytesSent));
}

static ObjectInstance* shard_socket_Bind(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    const wchar_t* ip_w = context.Args[1]->AsString();
    int64_t port = context.Args[2]->AsInteger();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(false);

    std::string ip_narrow = strings::WideToUtf8(ip_w);
    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_port = htons(static_cast<u_short>(port));

    if (ip_narrow.empty() || ip_narrow == "0.0.0.0")
    {
        service.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, ip_narrow.data(), &service.sin_addr) <= 0)
            return context.Collector.FromValue(false);
    }

    int result = bind(socket_handle, reinterpret_cast<sockaddr*>(&service), sizeof(service));
    return context.Collector.FromValue(result != SOCKET_ERROR_VAL);
}

static ObjectInstance* shard_socket_Listen(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    int64_t backlog = context.Args[1]->AsInteger();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(false);

    int result = listen(socket_handle, static_cast<int>(backlog));
    return context.Collector.FromValue(result != SOCKET_ERROR_VAL);
}

static ObjectInstance* shard_socket_Accept(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    socket_t server_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (server_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Invalid server socket handle");

    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    socket_t client_handle = accept(server_handle, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
    if (client_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Failed to accept client connection");

    ObjectInstance* client_instance = context.Collector.AllocateInstance(shard_socket);
    client_instance->SetField(shard_socket_handle->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(client_handle)));
    return client_instance;
}

static ObjectInstance* shard_socket_AcceptAsync(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    socket_t server_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (server_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Invalid server socket handle");

    struct State
    {
        ObjectRef InstanceRef;
        socket_t ClientHandle = INVALID_SOCKET_VAL;
        bool Failed = false;
    };

    auto state = std::make_shared<State>();
    state->InstanceRef = ObjectRef(instance);

    auto scopeState = detail::CreateAsyncScopeState(context, shard_socket);
    ObjectInstance* task = scopeState->task;
    AsyncValueScope<ObjectInstance*> scope(scopeState);

    scope.RunOnThreadPool([state, server_handle]()
    {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        socket_t client_handle = accept(server_handle, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        if (client_handle == INVALID_SOCKET_VAL)
        {
            state->Failed = true;
            return;
        }

        state->ClientHandle = client_handle;
    }, [stateScope = scope.ShareState(), state]() mutable
    {
        AsyncValueScope<ObjectInstance*> async(stateScope);

        if (state->Failed)
        {
            async.Fail(L"Failed to accept client connection");
            return;
        }

        ObjectInstance* client_instance = async.Collector().AllocateInstance(shard_socket);
        client_instance->SetField(shard_socket_handle->SlotIndex,
            async.Collector().FromValue(static_cast<std::int64_t>(state->ClientHandle)));
        async.Complete(client_instance);
    });

    return task;
}

static ObjectInstance* shard_socket_Receive(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    int symbols_requested = static_cast<int>(context.Args[1]->AsInteger());
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle->SlotIndex)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL || symbols_requested <= 0)
        return context.Collector.FromValue(L"");

    std::size_t bytes_to_read = symbols_requested * sizeof(wchar_t);
    std::vector<char> byte_buffer(bytes_to_read);

    int bytesReceived = recv(socket_handle, byte_buffer.data(), static_cast<int>(bytes_to_read), 0);
    if (bytesReceived <= 0)
        return context.Collector.FromValue(L"");

    std::size_t symbols_received = bytesReceived / sizeof(wchar_t);
    if (symbols_received == 0)
        return context.Collector.FromValue(L"");

    std::wstring w_buffer(
        reinterpret_cast<const wchar_t*>(byte_buffer.data()),
        symbols_received
    );

    return context.Collector.FromValue(w_buffer);
}

static ObjectInstance* shard_socket_Close(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    int64_t socket_handle = instance->GetField(shard_socket_handle->SlotIndex)->AsInteger();

    if (socket_handle != INVALID_SOCKET_VAL)
    {
        close_socket_native(static_cast<socket_t>(socket_handle));
        instance->SetField(shard_socket_handle->SlotIndex, context.Collector.FromValue(static_cast<int64_t>(INVALID_SOCKET_VAL)));
    }

    return nullptr;
}

static ObjectInstance* shard_socket_Dispose(const CallState& context) noexcept
{
    return shard_socket_Close(context);
}

// ============================================================================
// Stream symbol lookup helpers
// ============================================================================

static void EnsureStreamSymbols(SymbolTable* table)
{
    if (g_Stream_IStream != nullptr)
        return;

    g_Stream_IStream = SemanticModel::FindTypeByName(table, L"io.IStream");
    g_Stream_IReadableStream = SemanticModel::FindTypeByName(table, L"io.IReadableStream");
    g_Stream_IWritableStream = SemanticModel::FindTypeByName(table, L"io.IWritableStream");

    g_Stream_CancellationToken = SemanticModel::FindTypeByName(table, L"async.CancellationToken");
    if (g_Stream_CancellationToken != nullptr)
        g_Stream_CancellationToken_SourceField = SemanticModel::FindFieldByName(g_Stream_CancellationToken, L"_source");

    TypeSymbol* sourceType = SemanticModel::FindTypeByName(table, L"async.CancellationTokenSource");
    if (sourceType != nullptr)
        g_Stream_CancellationTokenSource_CanceledField = SemanticModel::FindFieldByName(sourceType, L"_canceled");
}

static bool IsStreamCancellationRequested(ObjectInstance* token)
{
    if (token == nullptr || token == GarbageCollector::NullInstance)
        return false;

    ObjectInstance* source = token->GetField(g_Stream_CancellationToken_SourceField->SlotIndex);
    if (source == nullptr || source == GarbageCollector::NullInstance)
        return false;

    ObjectInstance* canceled = source->GetField(g_Stream_CancellationTokenSource_CanceledField->SlotIndex);
    if (canceled == nullptr || canceled == GarbageCollector::NullInstance)
        return false;

    return canceled->AsInteger() != 0;
}

static socket_t GetSocketStreamHandle(ObjectInstance* instance)
{
    ObjectInstance* handle = instance->GetField(g_SocketStream_Handle->SlotIndex);
    if (handle == nullptr || handle == GarbageCollector::NullInstance)
        return INVALID_SOCKET_VAL;

    return static_cast<socket_t>(handle->AsInteger());
}

static void CloseSocketStream(ObjectInstance* instance, GarbageCollector& gc)
{
    int64_t socket_handle = instance->GetField(g_SocketStream_Handle->SlotIndex)->AsInteger();
    if (socket_handle != INVALID_SOCKET_VAL)
    {
        close_socket_native(static_cast<socket_t>(socket_handle));
        instance->SetField(g_SocketStream_Handle->SlotIndex, gc.FromValue(static_cast<int64_t>(INVALID_SOCKET_VAL)));
    }

    instance->SetField(g_SocketStream_IsOpen->SlotIndex, gc.FromValue(false));
}

static ObjectInstance* MakeCanceledTask(const CallState& context)
{
    return shard::FaultedTask(context, L"Operation canceled.");
}

// ============================================================================
// SocketStream
// ============================================================================

static ObjectInstance* shard_socketStream_InitDefault(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (!InitNetwork())
        throw std::runtime_error("Failed to init inet.");

    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VAL)
        throw std::runtime_error("Failed to create socket handle.");

    instance->SetField(g_SocketStream_Handle->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(sock)));
    instance->SetField(g_SocketStream_IsOpen->SlotIndex, context.Collector.FromValue(true));
    return instance;
}

static ObjectInstance* shard_socketStream_InitFromSocket(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* socket = context.Args[1];

    int64_t handle = socket->GetField(shard_socket_handle->SlotIndex)->AsInteger();
    instance->SetField(g_SocketStream_Handle->SlotIndex, context.Collector.FromValue(handle));
    instance->SetField(g_SocketStream_IsOpen->SlotIndex, context.Collector.FromValue(handle != INVALID_SOCKET_VAL));
    return instance;
}

static ObjectInstance* shard_socketStream_Connect(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    const wchar_t* ip_w = context.Args[1]->AsString();
    int64_t port = context.Args[2]->AsInteger();
    socket_t socket_handle = GetSocketStreamHandle(instance);

    if (socket_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Invalid socket handle.");

    std::string ip_narrow = strings::WideToUtf8(ip_w);
    sockaddr_in clientService{};
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, ip_narrow.data(), &clientService.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP address.");

    int result = connect(socket_handle,
        reinterpret_cast<sockaddr*>(&clientService),
        sizeof(clientService));

    return context.Collector.FromValue(result != SOCKET_ERROR_VAL);
}

static ObjectInstance* shard_socketStream_Read(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (!instance->GetField(g_SocketStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot read from a closed stream.");
    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    socket_t socket_handle = GetSocketStreamHandle(instance);
    if (socket_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Invalid socket handle.");

    std::vector<char> temp(static_cast<std::size_t>(count));
    int bytesReceived = recv(socket_handle, temp.data(), static_cast<int>(count), 0);
    if (bytesReceived < 0)
        throw std::runtime_error("Socket receive failed.");

    std::int64_t read = static_cast<std::int64_t>(bytesReceived);
    for (std::int64_t i = 0; i < read; ++i)
    {
        buffer->SetElement(static_cast<std::size_t>(offset + i),
            context.Collector.FromValue(static_cast<std::uint8_t>(temp[static_cast<std::size_t>(i)])));
    }

    return context.Collector.FromValue(read);
}

static ObjectInstance* shard_socketStream_Write(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (!instance->GetField(g_SocketStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot write to a closed stream.");
    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    socket_t socket_handle = GetSocketStreamHandle(instance);
    if (socket_handle == INVALID_SOCKET_VAL)
        throw std::runtime_error("Invalid socket handle.");

    std::vector<char> temp(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i)
    {
        ObjectInstance* element = buffer->GetElement(static_cast<std::size_t>(offset + i));
        temp[static_cast<std::size_t>(i)] = static_cast<char>(element->AsByte());
    }

    int bytesSent = send(socket_handle, temp.data(), static_cast<int>(count), 0);
    if (bytesSent < 0)
        throw std::runtime_error("Socket send failed.");

    return nullptr;
}

static ObjectInstance* shard_socketStream_Flush(const CallState& context) noexcept(false)
{
    (void)context;
    return nullptr;
}

static ObjectInstance* shard_socketStream_Close(const CallState& context) noexcept(false)
{
    CloseSocketStream(context.Args[0], context.Collector);
    return nullptr;
}

static ObjectInstance* shard_socketStream_Dispose(const CallState& context) noexcept(false)
{
    return shard_socketStream_Close(context);
}

// ============================================================================
// SocketStream async helpers
// ============================================================================

static ObjectInstance* shard_socketStream_ReadAsync_Impl(const CallState& context, ObjectInstance* token)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (IsStreamCancellationRequested(token))
    {
        ObjectInstance* task = context.Collector.AllocateGeneric(CLASS_VALUETASK, std::vector<TypeSymbol*>{ TYPE_INT });
        task->IsTaskLike = true;
        SetTaskState(task, CLASS_VALUETASK_StateField, AsyncState::FAULTED, context.Collector);
        task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex,
            shard::CreateRuntimeException(context.Collector, L"Operation canceled."));
        return task;
    }

    struct State
    {
        ObjectRef InstanceRef;
        ObjectRef BufferRef;
        ObjectInstance* Token = nullptr;
        std::int64_t Offset = 0;
        std::int64_t Count = 0;
        std::int64_t Result = 0;
        bool Canceled = false;
    };

    auto state = std::make_shared<State>();
    state->InstanceRef = ObjectRef(instance);
    state->BufferRef = ObjectRef(buffer);
    state->Token = token;
    state->Offset = offset;
    state->Count = count;

    return shard::DoValueTask<std::int64_t>(context, [state](shard::AsyncValueScope<std::int64_t> async)
    {
        shard::GarbageCollector* collector = &async.Collector();
        async.RunOnThreadPool(
            [state, collector]()
            {
                if (IsStreamCancellationRequested(state->Token))
                {
                    state->Canceled = true;
                    return;
                }

                socket_t socket_handle = GetSocketStreamHandle(state->InstanceRef.Instance);
                if (socket_handle == INVALID_SOCKET_VAL)
                {
                    state->Canceled = true;
                    return;
                }

                std::vector<char> temp(static_cast<std::size_t>(state->Count));
                int bytesReceived = recv(socket_handle, temp.data(), static_cast<int>(state->Count), 0);
                if (bytesReceived < 0)
                {
                    state->Canceled = true;
                    return;
                }

                state->Result = static_cast<std::int64_t>(bytesReceived);
                for (std::int64_t i = 0; i < state->Result; ++i)
                {
                    state->BufferRef.Instance->SetElement(static_cast<std::size_t>(state->Offset + i),
                        collector->FromValue(static_cast<std::uint8_t>(temp[static_cast<std::size_t>(i)])));
                }
            },
            [stateScope = async.ShareState(), state]() mutable
            {
                shard::AsyncValueScope<std::int64_t> async(stateScope);

                if (state->Canceled || IsStreamCancellationRequested(state->Token))
                {
                    async.Fail(L"Operation canceled.");
                    return;
                }

                async.Complete(state->Result);
            });
    });
}

static ObjectInstance* shard_socketStream_WriteAsync_Impl(const CallState& context, ObjectInstance* token)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (IsStreamCancellationRequested(token))
        return MakeCanceledTask(context);

    struct State
    {
        ObjectRef InstanceRef;
        ObjectRef BufferRef;
        ObjectInstance* Token = nullptr;
        std::int64_t Offset = 0;
        std::int64_t Count = 0;
        bool Failed = false;
        bool Canceled = false;
    };

    auto state = std::make_shared<State>();
    state->InstanceRef = ObjectRef(instance);
    state->BufferRef = ObjectRef(buffer);
    state->Token = token;
    state->Offset = offset;
    state->Count = count;

    return shard::DoAsync(context, [state](shard::AsyncScope async)
    {
        async.RunOnThreadPool(
            [state]()
            {
                if (IsStreamCancellationRequested(state->Token))
                {
                    state->Canceled = true;
                    return;
                }

                socket_t socket_handle = GetSocketStreamHandle(state->InstanceRef.Instance);
                if (socket_handle == INVALID_SOCKET_VAL)
                {
                    state->Failed = true;
                    return;
                }

                std::vector<char> temp(static_cast<std::size_t>(state->Count));
                for (std::int64_t i = 0; i < state->Count; ++i)
                {
                    ObjectInstance* element = state->BufferRef.Instance->GetElement(static_cast<std::size_t>(state->Offset + i));
                    temp[static_cast<std::size_t>(i)] = static_cast<char>(element->AsByte());
                }

                int bytesSent = send(socket_handle, temp.data(), static_cast<int>(state->Count), 0);
                if (bytesSent < 0)
                    state->Failed = true;
            },
            [stateScope = async.ShareState(), state]() mutable
            {
                shard::AsyncScope async(stateScope);

                if (state->Canceled || IsStreamCancellationRequested(state->Token))
                {
                    async.Fail(L"Operation canceled.");
                    return;
                }

                if (state->Failed)
                {
                    async.Fail(L"Socket send failed.");
                    return;
                }

                async.Complete();
            });
    });
}

static ObjectInstance* shard_socketStream_FlushAsync_Impl(const CallState& context, ObjectInstance* token)
{
    if (IsStreamCancellationRequested(token))
        return MakeCanceledTask(context);

    return shard::DoAsync(context, [=](shard::AsyncScope async)
    {
        async.Complete();
    });
}

static ObjectInstance* shard_socketStream_ReadAsync(const CallState& context) noexcept(false)
{
    return shard_socketStream_ReadAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_socketStream_ReadAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_socketStream_ReadAsync_Impl(context, context.Args[4]);
}

static ObjectInstance* shard_socketStream_WriteAsync(const CallState& context) noexcept(false)
{
    return shard_socketStream_WriteAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_socketStream_WriteAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_socketStream_WriteAsync_Impl(context, context.Args[4]);
}

static ObjectInstance* shard_socketStream_FlushAsync(const CallState& context) noexcept(false)
{
    return shard_socketStream_FlushAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_socketStream_FlushAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_socketStream_FlushAsync_Impl(context, context.Args[1]);
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.socket";
    lib.Description = L"ShardScript TCP socket library";
    lib.Version = L"0.2.0";

    static const shard::ShardLibDependencyInfo deps[] =
    {
        { L"shard.streams", L"0.1.0" }
    };

    lib.Dependencies = deps;
    lib.DependenciesLength = sizeof(deps) / sizeof(deps[0]);
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> netNamespace(context, L"net");
    SymbolBuilder<ClassSymbol> tcpSocket = netNamespace.AddClass(L"Socket");
    tcpSocket.Implements(TRAIT_DISPOSABLE);
    
    shard_socket = tcpSocket;
    shard_socket_handle = tcpSocket.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    tcpSocket.AddInit()
        .SetCallback(&shard_socket_init);

    tcpSocket.AddMethod(L"Connect", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"ip", TYPE_STRING)
        .AddParameter(L"port", TYPE_INT)
        .SetCallback(&shard_socket_Connect);

    tcpSocket.AddMethod(L"Send", TYPE_INT, LINK_INSTANCE)
        .AddParameter(L"data", TYPE_STRING)
        .SetCallback(&shard_socket_Send);

    tcpSocket.AddMethod(L"Bind", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"ip", TYPE_STRING)
        .AddParameter(L"port", TYPE_INT)
        .SetCallback(&shard_socket_Bind);

    tcpSocket.AddMethod(L"Listen", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"backlog", TYPE_INT)
        .SetCallback(&shard_socket_Listen);

    tcpSocket.AddMethod(L"Accept", shard_socket, LINK_INSTANCE)
        .SetCallback(&shard_socket_Accept);

    tcpSocket.AddMethod(L"Receive", TYPE_STRING, LINK_INSTANCE)
        .AddParameter(L"bufferSize", TYPE_INT)
        .SetCallback(&shard_socket_Receive);

    shard_socket_close = tcpSocket.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_socket_Close);

    tcpSocket.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_socket_Dispose)
        .IsImplementationOf(TRAIT_DISPOSABLE_Dispose);

    // --- stream symbols ---
    EnsureStreamSymbols(context.GetSemanticModel().Table.get());

    SymbolFactory factory(context.GetSemanticModel().Table.get());
    GenericTypeSymbol* valueTaskOfInt = factory.GenericType(CLASS_VALUETASK, { { L"T", TYPE_INT } });
    GenericTypeSymbol* valueTaskOfSocket = factory.GenericType(CLASS_VALUETASK, { { L"T", shard_socket } });

    tcpSocket.AddMethod(L"AcceptAsync", valueTaskOfSocket, LINK_INSTANCE)
        .SetCallback(&shard_socket_AcceptAsync);

    // --- class SocketStream ---
    {
        SymbolBuilder<ClassSymbol> socketStreamClass = netNamespace.AddClass(L"SocketStream");
        g_SocketStream = socketStreamClass;
        socketStreamClass.Implements(static_cast<InterfaceSymbol*>(g_Stream_IReadableStream));
        socketStreamClass.Implements(static_cast<InterfaceSymbol*>(g_Stream_IWritableStream));
        socketStreamClass.Implements(TRAIT_DISPOSABLE);

        g_SocketStream_Handle = socketStreamClass
            .AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

        g_SocketStream_IsOpen = socketStreamClass
            .AddField(L"_isOpen", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        socketStreamClass.AddInit()
            .SetCallback(&shard_socketStream_InitDefault);

        socketStreamClass.AddInit()
            .AddParameter(L"socket", shard_socket)
            .SetCallback(&shard_socketStream_InitFromSocket);

        socketStreamClass.AddMethod(L"Connect", TYPE_BOOL, LINK_INSTANCE)
            .AddParameter(L"ip", TYPE_STRING)
            .AddParameter(L"port", TYPE_INT)
            .SetCallback(&shard_socketStream_Connect);

        socketStreamClass.AddMethod(L"Read", TYPE_INT, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_socketStream_Read);

        socketStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_socketStream_ReadAsync);

        socketStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_socketStream_ReadAsync_Cancel);

        socketStreamClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_socketStream_Write);

        socketStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_socketStream_WriteAsync);

        socketStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_socketStream_WriteAsync_Cancel);

        socketStreamClass.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_socketStream_Flush);

        socketStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .SetCallback(&shard_socketStream_FlushAsync);

        socketStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_socketStream_FlushAsync_Cancel);

        socketStreamClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_socketStream_Close);

        socketStreamClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_socketStream_Dispose);

        socketStreamClass.DeclareGlobal();
    }
}