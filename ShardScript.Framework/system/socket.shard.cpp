#include <ShardScript.hpp>
#include <string>
#include <cstdint>
#include <vector>

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

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.socket";
    lib.Description = L"ShardScript TCP socket library";
    lib.Version = L"0.2.0";
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
}