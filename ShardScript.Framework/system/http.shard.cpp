#include <ShardScript.hpp>

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>

#include <uv.h>

#include <shard/runtime/EventLoop.hpp>

#ifndef _WIN32
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include "httplib.h"

using namespace shard;

TypeSymbol* shard_HttpServer = nullptr;
FieldSymbol* shard_HttpServer_ClientPtrField = nullptr;

TypeSymbol* shard_HttpClient = nullptr;
FieldSymbol* shard_HttpClient_ClientPtrField = nullptr;

TypeSymbol* shard_HttpResponse = nullptr;
FieldSymbol* shard_HttpResponse_StatusField = nullptr;
FieldSymbol* shard_HttpResponse_BodyField = nullptr;

// ============================================================================
// Async HTTP server support
// ============================================================================

struct HttpServerRequest
{
    ObjectInstance* Handler = nullptr;
    std::string RequestBody;
    httplib::Response* Response = nullptr;

    std::mutex Mutex;
    std::condition_variable CV;
    bool Done = false;
    bool ResponseWritten = false;

    bool Faulted = false;
    int StatusCode = 200;
    std::string ErrorMessage;
    std::string ResponseBody;
};

struct HttpServerContext
{
    ~HttpServerContext()
    {
        for (ObjectInstance* handler : Handlers)
        {
            if (handler != nullptr && handler != GarbageCollector::NullInstance)
                handler->DecrementReference();
        }
    }

    ApplicationDomain* Domain = nullptr;
    httplib::Server* Server = nullptr;
    uv_async_t WakeHandle{};

    std::mutex Mutex;
    std::condition_variable CV;
    std::deque<std::shared_ptr<HttpServerRequest>> Pending;

    std::atomic<bool> Stopped{ false };
    std::thread ListenThread;

    std::vector<ObjectInstance*> Handlers;
};

static HttpServerContext* GetServerContext(ObjectInstance* instance)
{
    if (instance == nullptr)
        return nullptr;

    ObjectInstance* handleVal = instance->GetField(shard_HttpServer_ClientPtrField->SlotIndex);
    if (handleVal == nullptr || handleVal == GarbageCollector::NullInstance)
        return nullptr;

    return static_cast<HttpServerContext*>(handleVal->AsNint());
}

static void ProcessServerRequest(HttpServerContext* ctx, HttpServerRequest* request)
{
    VirtualMachine& vm = ctx->Domain->GetVirtualMachine();
    GarbageCollector& gc = ctx->Domain->GetGarbageCollector();

    ObjectInstance* bodyObj = gc.FromValue(strings::Utf8ToWide(request->RequestBody));

    MethodSymbol* handlerMethod = request->Handler->DelegateTarget;
    CallStackFrame* rootFrame = vm.PushFrame(handlerMethod);
    rootFrame->PushStack(request->Handler);

    ObjectInstance* responseBody = nullptr;
    try
    {
        ObjectInstance* handlerArgs[] = { bodyObj };
        responseBody = vm.InvokeMethod(handlerMethod, handlerArgs, 1);
    }
    catch (const std::exception& ex)
    {
        request->Faulted = true;
        request->ErrorMessage = ex.what();
    }

    vm.PopFrame(); // synthetic root

    if (!request->Faulted && responseBody != nullptr)
    {
        request->ResponseBody = strings::WideToUtf8(responseBody->AsString());
        request->StatusCode = 200;
        gc.DestroyInstance(responseBody);
    }
    else if (!request->Faulted)
    {
        request->Faulted = true;
        request->ErrorMessage = "HTTP handler did not return a response body";
    }

    {
        std::lock_guard lock(request->Mutex);
        request->Done = true;
        request->CV.notify_one();
    }
}

static void ServerWakeCallback(uv_async_t* handle)
{
    HttpServerContext* ctx = static_cast<HttpServerContext*>(handle->data);
    if (ctx == nullptr)
        return;

    std::deque<std::shared_ptr<HttpServerRequest>> requests;
    {
        std::lock_guard lock(ctx->Mutex);
        requests = std::move(ctx->Pending);
    }

    for (auto& req : requests)
        ProcessServerRequest(ctx, req.get());
}

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

    std::string u8BaseUrl = strings::WideToUtf8(wideBaseUrl);
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

    std::string u8Path = strings::WideToUtf8(widePath);

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

    HttpServerContext* ctx = new HttpServerContext();
    ctx->Domain = &context.Domain;
    ctx->Server = new httplib::Server();

    uv_loop_t* loop = context.Domain.GetEventLoop().GetLoop();
    uv_async_init(loop, &ctx->WakeHandle, ServerWakeCallback);
    ctx->WakeHandle.data = ctx;

    instance->SetField(shard_HttpServer_ClientPtrField->SlotIndex, context.Collector.FromNint(ctx, true));
    return instance;
}

static ObjectInstance* shard_http_Server_Get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* widePathObj = context.Args[1];
    ObjectInstance* shardCallback = context.Args[2];

    HttpServerContext* ctx = GetServerContext(instance);
    if (ctx == nullptr || ctx->Server == nullptr)
        throw std::runtime_error("HttpServer: Server is disposed.");

    shardCallback->IncrementReference();
    ctx->Handlers.push_back(shardCallback);

    std::string path = strings::WideToUtf8(widePathObj->AsString());

    ctx->Server->Get(path, [ctx, shardCallback](const httplib::Request& req, httplib::Response& res)
    {
        auto request = std::make_shared<HttpServerRequest>();
        request->Handler = shardCallback;
        request->RequestBody = req.body;
        request->Response = &res;

        {
            std::lock_guard lock(ctx->Mutex);
            ctx->Pending.push_back(request);
        }

        uv_async_send(&ctx->WakeHandle);

        std::unique_lock lock(request->Mutex);
        request->CV.wait(lock, [request] { return request->Done; });

        if (request->Faulted)
        {
            res.status = 500;
            res.set_content(request->ErrorMessage, "text/plain; charset=utf-8");
        }
        else
        {
            res.status = request->StatusCode;
            res.set_content(request->ResponseBody, "text/plain; charset=utf-8");
        }

        request->ResponseWritten = true;
    });

    return nullptr;
}

static ObjectInstance* shard_http_Server_Listen(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* hostObj = context.Args[1];
    ObjectInstance* portObj = context.Args[2];

    HttpServerContext* ctx = GetServerContext(instance);
    if (ctx == nullptr || ctx->Server == nullptr)
        throw std::runtime_error("HttpServer: Server is disposed.");

    std::string host = strings::WideToUtf8(hostObj->AsString());
    int64_t port = portObj->AsInteger();

    ctx->Stopped = false;
    ctx->ListenThread = std::thread([ctx, host, port]()
    {
        ctx->Server->listen(host, static_cast<int>(port));
    });

    // Pump the domain event loop on this thread.  Request handlers are
    // marshalled from httplib's worker threads onto this loop so they run
    // on the same VM/ApplicationDomain as the rest of the program.
    ctx->Domain->GetEventLoop().Run();

    if (ctx->ListenThread.joinable())
        ctx->ListenThread.join();

    // Schedule context destruction; the close callback runs when the loop
    // processes the handle close.
    instance->SetField(shard_HttpServer_ClientPtrField->SlotIndex,
                       GarbageCollector::NullInstance);
    uv_close(reinterpret_cast<uv_handle_t*>(&ctx->WakeHandle),
             [](uv_handle_t* handle)
             {
                 HttpServerContext* ctx = static_cast<HttpServerContext*>(handle->data);
                 if (ctx != nullptr)
                 {
                     delete ctx->Server;
                     delete ctx;
                 }
             });

    return nullptr;
}

static ObjectInstance* shard_http_Server_Dispose(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    HttpServerContext* ctx = GetServerContext(instance);

    if (ctx != nullptr)
    {
        ctx->Stopped = true;

        if (ctx->Server != nullptr)
            ctx->Server->stop();

        ctx->Domain->GetEventLoop().Stop();

        // If Listen() has already returned we can clean up immediately;
        // otherwise Listen() will perform the cleanup after joining the thread.
        if (!ctx->ListenThread.joinable())
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&ctx->WakeHandle), nullptr);
            delete ctx->Server;
            delete ctx;
            instance->SetField(shard_HttpServer_ClientPtrField->SlotIndex,
                               GarbageCollector::NullInstance);
        }
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