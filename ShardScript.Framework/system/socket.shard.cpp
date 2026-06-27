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

static std::string thinify(const wchar_t* wstr)
{
    size_t length = wcslen(wstr) + 1;
    std::string narrow(length, '\0');
    size_t converted = 0;
    wcstombs_s(&converted, narrow.data(), length, wstr, _TRUNCATE);
    return narrow;
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

    instance->SetField(shard_socket_handle, context.Collector.FromValue(static_cast<std::int64_t>(sock)));
    return instance;
}

static ObjectInstance* shard_socket_Connect(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    const wchar_t* ip_w = context.Args[0]->AsString();
    int64_t port = context.Args[1]->AsInteger();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(false);

    std::string ip_narrow = thinify(ip_w);
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
    const wchar_t* data_w = context.Args[1]->AsString();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL)
        return context.Collector.FromValue(static_cast<int64_t>(-1));

    std::string data_narrow = thinify(data_w);
    int bytesSent = send(static_cast<socket_t>(socket_handle), data_narrow.data(), static_cast<int>(strlen(data_narrow.data())), 0);
    return context.Collector.FromValue(static_cast<int64_t>(bytesSent));
}

static ObjectInstance* shard_socket_Receive(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    int64_t buffer_size = context.Args[1]->AsInteger();
    socket_t socket_handle = static_cast<socket_t>(instance->GetField(shard_socket_handle)->AsInteger());

    if (socket_handle == INVALID_SOCKET_VAL || buffer_size <= 0)
        return context.Collector.FromValue(L"");

    std::vector<char> buffer(buffer_size + 1);
    int bytesReceived = recv(static_cast<socket_t>(socket_handle), buffer.data(), static_cast<int>(buffer_size), 0);

    if (bytesReceived <= 0)
        return context.Collector.FromValue(L"");

    buffer[bytesReceived] = '\0';
    std::wstring w_buffer(buffer.begin(), buffer.end());
    return context.Collector.FromValue(w_buffer.data());
}

static ObjectInstance* shard_socket_Close(const CallState& context) noexcept
{
    int64_t socket_handle = context.Args[0]->AsInteger();
    if (socket_handle != INVALID_SOCKET_VAL)
    {
        close_socket_native(static_cast<socket_t>(socket_handle));
    }

    return nullptr;
}

static ObjectInstance* shard_socket_Dispose(const CallState& context) noexcept
{
    context.Runtimer.InvokeMethod(shard_socket_close, {});
    return nullptr;
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
    
    shard_socket_handle = tcpSocket.AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    tcpSocket.AddInit()
        .AddParameter(L"port", TYPE_INT)
        .SetCallback(&shard_socket_init);

    tcpSocket.AddMethod(L"Connect", tcpSocket, LINK_INSTANCE)
        .AddParameter(L"ip", TYPE_STRING)
        .AddParameter(L"port", TYPE_INT)
        .SetCallback(&shard_socket_Connect);

    tcpSocket.AddMethod(L"Send", TYPE_INT, LINK_INSTANCE)
        .AddParameter(L"data", TYPE_STRING)
        .SetCallback(&shard_socket_Send);

    tcpSocket.AddMethod(L"Receive", TYPE_STRING, LINK_INSTANCE)
        .AddParameter(L"bufferSize", TYPE_INT)
        .SetCallback(&shard_socket_Receive);

    shard_socket_close = tcpSocket.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_socket_Close);

    tcpSocket.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_socket_Dispose);
}