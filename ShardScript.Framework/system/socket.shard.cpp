#include <ShardScript.hpp>

#include <string>
#include <cstdint>
#include <vector>

#ifdef _WIN32

    // Windows
    #include <WinSock2.h>
    #include <Windows.h>
    #include <ws2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")
#else

    // Linux (POSIX)
    #include <sys/types.h>
    #include <sys/sockhandle_t.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cerrno>
#endif

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.socket";
	lib.Description = L"ShardScript socket lib";
	lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{

}