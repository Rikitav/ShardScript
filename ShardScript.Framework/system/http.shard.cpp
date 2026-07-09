#include <ShardScript.hpp>

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

using namespace shard;

namespace
{
    static inline std::string WToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
            return {};

#ifdef _WIN32
        const int size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
            static_cast<int>(wstr.size()),
            nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return {};

        std::string narrow(static_cast<std::size_t>(size), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
            static_cast<int>(wstr.size()),
            narrow.data(), size, nullptr, nullptr);
        return narrow;
#else
        std::string narrow(wstr.size() * 4 + 1, '\0');
        std::wcstombs(narrow.data(), wstr.c_str(), narrow.size());
        narrow.resize(std::strlen(narrow.c_str()));
        return narrow;
#endif
    }

    static inline std::string WToUtf8(const wchar_t* wstr)
    {
        if (wstr == nullptr)
            return {};

        return WToUtf8(std::wstring(wstr));
    }

    static inline std::wstring Utf8ToW(const std::string& narrow)
    {
        if (narrow.empty())
            return {};

#ifdef _WIN32
        const int size = ::MultiByteToWideChar(CP_UTF8, 0, narrow.data(),
            static_cast<int>(narrow.size()),
            nullptr, 0);
        if (size <= 0)
            return {};

        std::wstring wide(static_cast<std::size_t>(size), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, narrow.data(),
            static_cast<int>(narrow.size()),
            wide.data(), size);
        return wide;
#else
        std::wstring wide(narrow.size(), L'\0');
        std::mbstowcs(wide.data(), narrow.c_str(), wide.size());
        wide.resize(std::wcslen(wide.c_str()));
        return wide;
#endif
    }
}

TypeSymbol* shard_HttpServer = nullptr;
FieldSymbol* shard_HttpServer_ClientPtrField = nullptr;

TypeSymbol* shard_HttpClient = nullptr;
FieldSymbol* shard_HttpClient_ClientPtrField = nullptr;

TypeSymbol* shard_HttpResponse = nullptr;
FieldSymbol* shard_HttpResponse_StatusField = nullptr;
FieldSymbol* shard_HttpResponse_BodyField = nullptr;

static httplib::Client* GetClientPtr(ObjectInstance* instance)
{
    if (!instance)
        return nullptr;

    ObjectInstance* handleVal = instance->GetField(shard_HttpClient_ClientPtrField->SlotIndex);
    if (!handleVal)
        return nullptr;

    return reinterpret_cast<httplib::Client*>(handleVal->AsNint());
}

static httplib::Server* GetServerPtr(ObjectInstance* instance)
{
    if (!instance)
        return nullptr;

    ObjectInstance* handleVal = instance->GetField(shard_HttpServer_ClientPtrField->SlotIndex);
    if (!handleVal)
        return nullptr;

    return reinterpret_cast<httplib::Server*>(handleVal->AsNint());
}

// ============================================================================
// class HttpResponse
// ============================================================================

static ObjectInstance* shard_http_Response_Init(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* status = context.Args[1];
    ObjectInstance* body = context.Args[2];

    instance->SetField(shard_HttpResponse_StatusField->SlotIndex, status);
    instance->SetField(shard_HttpResponse_BodyField->SlotIndex, body);
    return instance;
}

// ============================================================================
// class HttpClient
// ============================================================================

static ObjectInstance* shard_http_Client_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring wideBaseUrl = context.Args[1]->AsString();

    std::string u8BaseUrl = WToUtf8(wideBaseUrl);
    httplib::Client* client = new httplib::Client(u8BaseUrl);

    client->set_connection_timeout(std::chrono::seconds(5));
    client->set_read_timeout(std::chrono::seconds(5));

    instance->SetField(shard_HttpClient_ClientPtrField->SlotIndex, context.Collector.FromNint(client, true));
    return instance;
}

static ObjectInstance* shard_http_Client_Get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring widePath = context.Args[1]->AsString();

    httplib::Client* client = GetClientPtr(instance);
    if (client == nullptr)
        throw std::runtime_error("HttpClient: Client is disposed.");

    std::string u8Path = WToUtf8(widePath);

    httplib::Result res = client->Get(u8Path);
    if (res == nullptr)
        throw std::runtime_error("HTTP Request Failed: " + httplib::to_string(res.error()));

    int64_t statusCode = res->status;
    std::string u8Body = res->body;
    std::wstring wideBody(u8Body.begin(), u8Body.end());

    ObjectInstance* responseInstance = context.Collector.AllocateInstance(shard_HttpResponse);
    ObjectInstance* statusVal = context.Collector.FromValue(statusCode);
    ObjectInstance* bodyVal = context.Collector.FromValue(wideBody);

    CallState initContext = context; // copying context
    ObjectInstance* newargs[] = { responseInstance, statusVal, bodyVal };
    initContext.Args = newargs;
    shard_http_Response_Init(initContext);

    return responseInstance;
}

static ObjectInstance* shard_http_Client_Post(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring widePath = context.Args[1]->AsString();
    std::wstring wideJson = context.Args[2]->AsString();

    httplib::Client* client = GetClientPtr(instance);
    if (client == nullptr)
        throw std::runtime_error("HttpClient: Client is disposed.");

    std::string u8Path(widePath.begin(), widePath.end());
    std::string u8Json(wideJson.begin(), wideJson.end());

    httplib::Result res = client->Post(u8Path, u8Json, "application/json");
    if (res == nullptr)
        throw std::runtime_error("HTTP POST Failed: " + httplib::to_string(res.error()));

    ObjectInstance* responseInstance = context.Collector.AllocateInstance(shard_HttpResponse);
    ObjectInstance* statusVal = context.Collector.FromValue(static_cast<int64_t>(res->status));
    ObjectInstance* bodyVal = context.Collector.FromValue(std::wstring(res->body.begin(), res->body.end()));

    CallState initContext = context; // copying context
    ObjectInstance* newargs[] = { responseInstance, statusVal, bodyVal };
    initContext.Args = newargs;
    shard_http_Response_Init(initContext);

    return responseInstance;
}

static ObjectInstance* shard_http_Client_Dispose(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    httplib::Client* client = GetClientPtr(instance);

    if (client != nullptr)
    {
        delete client;
        instance->SetField(shard_HttpClient_ClientPtrField->SlotIndex, context.Collector.FromNint(nullptr, false));
    }
    
    return nullptr;
}

// ============================================================================
// class HttpServer
// ============================================================================

static ObjectInstance* shard_http_Server_Init(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    httplib::Server* server = new httplib::Server();

    instance->SetField(shard_HttpServer_ClientPtrField->SlotIndex, context.Collector.FromNint(server, true));
    return instance;
}

static ObjectInstance* shard_http_Server_Get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* widePathObj = context.Args[1];
    ObjectInstance* shardCallback = context.Args[2];

    httplib::Server* server = GetServerPtr(instance);
    if (server == nullptr)
        throw std::runtime_error("HttpServer: Server is disposed.");

    shardCallback->IncrementReference();
    std::string path = WToUtf8(widePathObj->AsString());

    server->Get(path, [shardCallback, &context](const httplib::Request& req, httplib::Response& res)
    {
		// Since this callback is executed in a different thread,
        // we need to create a new VirtualMachine instance for this thread,
        // to not interfere with the main thread's VM state.
        // Basically a dumbass hack, for until i make async and thread context switching possible.
		ApplicationDomain* appDomain = &context.Domain;
        VirtualMachine innerVm(appDomain);

        std::wstring wideBody = Utf8ToW(req.body);

        //CallStackFrame dumbFrame(&innerVm, nullptr, nullptr, nullptr);
        innerVm.PushFrame(nullptr);
        innerVm.InvokeMethod(shardCallback->DelegateTarget, { context.Collector.FromValue(wideBody) });
        
        CallStackFrame* dumbFrame = innerVm.CurrentFrame();
        ObjectInstance* responceBody = dumbFrame->PopStack();
        
		if (dumbFrame->InterruptionReason == FrameInterruptionReason::ExceptionRaised)
		{
			res.set_content(WToUtf8(dumbFrame->CurrentException->AsString()), "text/plain; charset=utf-8");
			res.status = 500;
		}
        else
        {
            res.set_content(WToUtf8(responceBody->AsString()), "text/plain; charset=utf-8");
            res.status = 200;
        }

        innerVm.GetGarbageCollector().DestroyInstance(responceBody);
    });

    return nullptr;
}

static ObjectInstance* shard_http_Server_Listen(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* hostObj = context.Args[1];
	ObjectInstance* portObj = context.Args[2];

    std::string host = WToUtf8(hostObj->AsString());
    int64_t port = portObj->AsInteger();

    httplib::Server* server = GetServerPtr(instance);
    if (server == nullptr)
        throw std::runtime_error("HttpServer: Server is disposed.");

	// block thread, this will run the server loop
    bool success = server->listen(host, static_cast<int>(port));
    if (!success)
        throw std::runtime_error("HttpServer: Failed to bind or listen on \"" + host + "\" :" + std::to_string(port));

    return nullptr;
}

static ObjectInstance* shard_http_Server_Dispose(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    httplib::Server* server = GetServerPtr(instance);

    if (server != nullptr)
    {
        server->stop();
        delete server;
    }

    return nullptr;
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.http";
    lib.Description = L"High-performance native HTTP client provider";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> httpNamespace(context, L"net");

    // --- class HttpResponse ---
    SymbolBuilder<ClassSymbol> respClass = httpNamespace.AddClass(L"HttpResponse");
    shard_HttpResponse = respClass;

	SymbolBuilder<PropertySymbol> statusCodeProp = respClass.AddProperty(L"StatusCode", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
    shard_HttpResponse_StatusField = statusCodeProp
        .AddBackingField();

    statusCodeProp.AddGetter()
        .SetCallback([](const CallState& context) { return context.Args[0]->GetField(shard_HttpResponse_StatusField->SlotIndex); });

    SymbolBuilder<PropertySymbol> bodyProp = respClass.AddProperty(L"Body", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);
    shard_HttpResponse_BodyField = bodyProp
        .AddBackingField();

    bodyProp.AddGetter()
        .SetCallback([](const CallState& context) { return context.Args[0]->GetField(shard_HttpResponse_BodyField->SlotIndex); });

    // --- class HttpClient ---
    SymbolBuilder<ClassSymbol> clientClass = httpNamespace.AddClass(L"HttpClient");
    shard_HttpClient = clientClass
        .Implements(TRAIT_DISPOSABLE);

    shard_HttpClient_ClientPtrField = clientClass
        .AddField(L"_clientPtr", TYPE_NINT, LINK_INSTANCE, ACS_PRIVATE);

    clientClass.AddInit()
        .AddParameter(L"baseUrl", TYPE_STRING)
        .SetCallback(&shard_http_Client_Init);

    clientClass.AddMethod(L"Get", shard_HttpResponse, LINK_INSTANCE)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_http_Client_Get);

    clientClass.AddMethod(L"Post", shard_HttpResponse, LINK_INSTANCE)
        .AddParameter(L"path", TYPE_STRING)
        .AddParameter(L"jsonPayload", TYPE_STRING)
        .SetCallback(&shard_http_Client_Post);

    clientClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
        .SetCallback(&shard_http_Client_Dispose)
        .IsImplementationOf(TRAIT_DISPOSABLE_Dispose);

    // --- class HttpServer ---
    SymbolBuilder<ClassSymbol> serverClass = httpNamespace.AddClass(L"HttpServer");
    shard_HttpServer = serverClass
        .Implements(TRAIT_DISPOSABLE);

    shard_HttpServer_ClientPtrField = serverClass
        .AddField(L"_nativeServer", TYPE_NINT, LINK_INSTANCE, ACS_PRIVATE);

    serverClass.AddInit()
        .SetCallback(&shard_http_Server_Init);

	SymbolFactory& factory = serverClass.GetFactory();
    std::vector<ParameterSymbol*> params = { factory.Parameter(L"requestBody", TYPE_STRING) };
    DelegateTypeSymbol* delegate = factory.Delegate(L"ServerGetCallback", TYPE_STRING, params);

    serverClass.AddMethod(L"Get", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"path", TYPE_STRING)
        .AddParameter(L"handler", delegate)
        .SetCallback(&shard_http_Server_Get);

    serverClass.AddMethod(L"Listen", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"host", TYPE_STRING)
        .AddParameter(L"port", TYPE_INT)
        .SetCallback(&shard_http_Server_Listen);

    serverClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
        .SetCallback(&shard_http_Server_Dispose)
        .IsImplementationOf(TRAIT_DISPOSABLE_Dispose);
}