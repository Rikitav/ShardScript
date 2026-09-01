#if !defined(SHARDSCRIPT_STATIC)

#include <ShardScript.hpp>
#include <shard/ShardScriptExtern.hpp>
#include <shard/runtime/NativeAsync.hpp>
#include <utilities/LibraryLoader.hpp>

#include <sstream>
#include <filesystem>
#include <wchar.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <mutex>

using namespace shard;

namespace
{
    thread_local std::wstring LastErrorMessage;

    static void SetLastShardError(const std::string& message)
    {
        LastErrorMessage = std::wstring(message.begin(), message.end());
    }

    static void SetLastShardWError(const std::wstring& message)
    {
        LastErrorMessage = message;
    }

    static void SetLastErrorFromException(const std::exception& ex)
    {
        SetLastShardError(ex.what());
    }

    static void SetLastUnhandledExceptionError(VirtualMachine& vm)
    {
        std::wstring error = L"Unhandled exception";
        const std::wstring& message = vm.GetUnhandledExceptionMessage();
        if (!message.empty())
            error += L": " + message;

        error += L"\nStack trace:\n" + vm.GetUnhandledExceptionStackTrace();
        SetLastShardWError(error);
    }

    static int CopyWString(const std::wstring& source, wchar_t* buffer, int bufferLen)
    {
        if (buffer != nullptr && bufferLen > 0)
        {
            std::size_t copyLen = (std::min)((std::size_t)bufferLen - 1, source.length());
            wcsncpy(buffer, source.c_str(), copyLen);
            buffer[copyLen] = L'\0';
        }

        return (int)source.length();
    }

    static SyntaxToken MakeToken(shard::TokenType type, const wchar_t* word = nullptr)
    {
        return SyntaxToken(type, word != nullptr ? std::wstring(word) : std::wstring(), TextLocation(), false);
    }

    // =========================================================================
    // Async task state registry
    // =========================================================================
    // C async callbacks receive the task ObjectInstance* as their handle.  The
    // internal AsyncScopeState is kept alive here so callbacks can complete,
    // fault, delay, await, etc. without holding a dangling raw pointer.

    std::mutex g_asyncStateMutex;
    std::unordered_map<ObjectInstance*, std::shared_ptr<detail::AsyncScopeState>> g_asyncStateRegistry;

    static std::shared_ptr<detail::AsyncScopeState> GetAsyncState(ObjectInstance* task)
    {
        std::lock_guard<std::mutex> lock(g_asyncStateMutex);
        auto it = g_asyncStateRegistry.find(task);
        if (it == g_asyncStateRegistry.end())
            return nullptr;
        return it->second;
    }

    static void RegisterAsyncState(ObjectInstance* task, std::shared_ptr<detail::AsyncScopeState> state)
    {
        std::lock_guard<std::mutex> lock(g_asyncStateMutex);
        g_asyncStateRegistry[task] = std::move(state);
    }

    static void UnregisterAsyncState(ObjectInstance* task)
    {
        std::lock_guard<std::mutex> lock(g_asyncStateMutex);
        g_asyncStateRegistry.erase(task);
    }

    static std::shared_ptr<detail::AsyncScopeState> GetAsyncStateChecked(ObjectInstance* task)
    {
        if (task == nullptr)
        {
            SetLastShardWError(L"task is null");
            return nullptr;
        }

        auto state = GetAsyncState(task);
        if (state == nullptr)
        {
            SetLastShardWError(L"async state not found for task");
            return nullptr;
        }

        return state;
    }

    // =========================================================================
    // Managed callbacks from host languages (e.g. C#)
    // =========================================================================

    typedef shard::ObjectInstance* (*ShardManagedMethodCallback)(
        shard::MethodSymbol* method,
        shard::ObjectInstance** args,
        int argsCount,
        void* userData,
        shard::GarbageCollector* collector);

    struct ManagedMethodCallbackEntry
    {
        ShardManagedMethodCallback Callback;
        void* UserData;
    };

    static std::unordered_map<shard::MethodSymbol*, ManagedMethodCallbackEntry> ManagedMethodCallbacks;

    static shard::ObjectInstance* InvokeManagedMethodCallback(const shard::CallState& context)
    {
        auto it = ManagedMethodCallbacks.find(context.Method);
        if (it == ManagedMethodCallbacks.end())
            return nullptr;

        const ManagedMethodCallbackEntry& entry = it->second;
        return entry.Callback(
            context.Method,
            context.Args.data(),
            static_cast<int>(context.Args.size()),
            entry.UserData,
            &context.Collector);
    }

    using ShardManagedCallStateCallback = shard::ObjectInstance* (*)(const shard::CallState* state, void* userData);

    struct ManagedCallStateCallbackEntry
    {
        ShardManagedCallStateCallback Callback;
        void* UserData;
    };

    static std::unordered_map<shard::MethodSymbol*, ManagedCallStateCallbackEntry> ManagedCallStateCallbacks;

    static shard::ObjectInstance* InvokeManagedCallStateCallback(const shard::CallState& context)
    {
        auto it = ManagedCallStateCallbacks.find(context.Method);
        if (it == ManagedCallStateCallbacks.end())
            return nullptr;

        const ManagedCallStateCallbackEntry& entry = it->second;
        return entry.Callback(&context, entry.UserData);
    }
}

extern "C"
{
    // =========================================================================
    // Error Handling
    // =========================================================================

    SHARD_API int Shard_GetLastError(wchar_t* buffer, int bufferLen)
    {
        if (buffer != nullptr && bufferLen > 0)
        {
            std::size_t copyLen = (std::min)((std::size_t)bufferLen - 1, LastErrorMessage.length());
            wcsncpy(buffer, LastErrorMessage.c_str(), copyLen);
            buffer[copyLen] = L'\0';
        }

        return (int)LastErrorMessage.length();
    }

    // =========================================================================
    // Compilation Context API
    // =========================================================================

    SHARD_API CompilationContext* Shard_CreateCompilationContext()
    {
        try
        {
            return new CompilationContext();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_DestroyCompilationContext(CompilationContext* ctx)
    {
        try
        {
            if (ctx != nullptr)
                delete ctx;

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddLibrary(CompilationContext* ctx, const wchar_t* path)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            if (path == nullptr)
            {
                SetLastShardWError(L"library path is null");
                return -1;
            }

            ctx->AddLib(std::filesystem::path(path));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddLibraries(CompilationContext* ctx, const wchar_t* const* paths, std::size_t count)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            if (paths == nullptr && count > 0)
            {
                SetLastShardWError(L"library paths array is null");
                return -1;
            }

            std::vector<std::filesystem::path> libraryPaths;
            libraryPaths.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
            {
                if (paths[i] == nullptr)
                {
                    SetLastShardWError(L"library path is null");
                    return -1;
                }
                libraryPaths.emplace_back(paths[i]);
            }

            ctx->AddLibraries(libraryPaths);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddSource(CompilationContext* ctx, const wchar_t* sourceName, const wchar_t* code, CompilationUnitOrigin origin)
    {
        try
        {
            if (ctx == nullptr || code == nullptr || sourceName == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            std::wstring sourceCode(code);
            StringStreamReader reader(sourceName, sourceCode);
            LexicalAnalyzer lexer(reader);

            ctx->EnrichTree(lexer, origin);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddSourceFile(CompilationContext* ctx, const wchar_t* filePath, CompilationUnitOrigin origin)
    {
        try
        {
            if (ctx == nullptr || filePath == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            FileReader reader(filePath);
            LexicalAnalyzer lexer(reader);

            ctx->EnrichTree(lexer, origin);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_Analyze(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            ctx->AnalyzeTree();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ApplicationDomain* Shard_Compile(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            return ctx->Compile().release();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ApplicationDomain* Shard_CompileAndRun(CompilationContext* ctx)
    {
        try
        {
            ApplicationDomain* domain = Shard_Compile(ctx);
            if (domain != nullptr)
            {
                domain->GetVirtualMachine().Run();
                if (domain->GetVirtualMachine().GetUnhandledException() != nullptr)
                {
                    SetLastUnhandledExceptionError(domain->GetVirtualMachine());
                    return nullptr;
                }

                // Pump the event loop so async behavior matches Shard_RunDomain.
                domain->GetEventLoop().Run();
            }

            return domain;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetEntryPoint(CompilationContext* ctx, int value)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            ctx->SetEntryPoint = value != 0;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetEntryPoint(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return 0;
            }

            return ctx->SetEntryPoint ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_SetPopExpressionStatement(CompilationContext* ctx, int value)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            ctx->SetPopExpressionStatement(value != 0);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_HasErrors(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 1;

            return ctx->GetDiagnosticsContext().AnyError ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 1;
        }
    }

    SHARD_API int Shard_GetErrorCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            const auto& diagnostics = ctx->GetDiagnosticsContext().Diagnostics;
            return static_cast<int>(std::count_if(diagnostics.begin(), diagnostics.end(),
                [](const Diagnostic& d) { return d.Severity == DiagnosticSeverity::Error; }));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_ResetDiagnostics(CompilationContext* ctx)
    {
        try
        {
            if (ctx != nullptr)
                ctx->GetDiagnosticsContext().Reset();

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetDiagnostics(CompilationContext* ctx, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            std::wstringstream ss;
            ctx->GetDiagnosticsContext().WriteDiagnostics(ss);
            std::wstring str = ss.str();

            if (buffer != nullptr && bufferLen > 0)
            {
                std::size_t copyLen = (std::min)((std::size_t)bufferLen - 1, str.length());
                wcsncpy(buffer, str.c_str(), copyLen);
                buffer[copyLen] = L'\0';
            }

            return (int)str.length();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    // =========================================================================
    // Application Domain API
    // =========================================================================

    SHARD_API int Shard_RunDomain(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetVirtualMachine().Run();
            if (domain->GetVirtualMachine().GetUnhandledException() != nullptr)
            {
                SetLastUnhandledExceptionError(domain->GetVirtualMachine());
                return -1;
            }

            // Pump the libuv event loop until all pending async operations complete.
            domain->GetEventLoop().Run();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_DestroyDomain(ApplicationDomain* domain)
    {
        try
        {
            if (domain != nullptr)
                delete domain;

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API VirtualMachine* Shard_GetVirtualMachine(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return nullptr;
            }

            return &domain->GetVirtualMachine();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API GarbageCollector* Shard_GetGarbageCollector(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return nullptr;
            }

            return &domain->GetGarbageCollector();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ProgramVirtualImage* Shard_GetProgram(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return nullptr;
            }

            return &domain->GetProgram();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_GetEntryPointMethod(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return nullptr;
            }

            return domain->GetProgram().EntryPoint;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetScriptArguments(ApplicationDomain* domain, const wchar_t* const* args, std::size_t count)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            std::vector<std::wstring> arguments;
            arguments.reserve(count);
            for (std::size_t i = 0; i < count; ++i)
                arguments.emplace_back(args[i] != nullptr ? args[i] : L"");

            domain->SetScriptArguments(std::move(arguments));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API std::size_t Shard_GetScriptArgumentCount(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
                return 0;

            return domain->GetScriptArguments().size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetScriptArgument(ApplicationDomain* domain, std::size_t index, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            const std::vector<std::wstring>& arguments = domain->GetScriptArguments();
            if (index >= arguments.size())
            {
                SetLastShardWError(L"script argument index out of range");
                return -1;
            }

            return CopyWString(arguments[index], buffer, bufferLen);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API std::size_t Shard_GetProgramDataSectionSize(ProgramVirtualImage* program)
    {
        try
        {
            if (program == nullptr)
                return 0;

            return program->DataSection.size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetProgramDataSectionByte(ProgramVirtualImage* program, std::size_t index)
    {
        try
        {
            if (program == nullptr)
                return -1;

            if (index >= program->DataSection.size())
                return -1;

            return static_cast<int>(program->DataSection[index]);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Virtual Machine API
    // =========================================================================

    SHARD_API int Shard_VMRun(VirtualMachine* vm)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            vm->Run();
            if (vm->GetUnhandledException() != nullptr)
            {
                SetLastUnhandledExceptionError(*vm);
                return -1;
            }

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMAbort(VirtualMachine* vm)
    {
        try
        {
            if (vm != nullptr)
                vm->Abort();

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMTerminateCallStack(VirtualMachine* vm)
    {
        try
        {
            if (vm != nullptr)
                vm->TerminateCallStack();

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_VMInvokeMethod(VirtualMachine* vm, MethodSymbol* method, ObjectInstance** args, int argCount)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return nullptr;
            }

            return vm->InvokeMethod(method, args, static_cast<std::size_t>(argCount));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_VMSetPendingTypeArguments(VirtualMachine* vm, TypeSymbol** typeArgs, std::size_t count)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            std::vector<TypeSymbol*> args;
            if (typeArgs != nullptr && count > 0)
                args.assign(typeArgs, typeArgs + count);

            vm->SetPendingTypeArguments(args);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMGetUnhandledException(VirtualMachine* vm, ObjectInstance** outException)
    {
        try
        {
            if (vm == nullptr || outException == nullptr)
            {
                SetLastShardWError(L"virtual machine or output pointer is null");
                return -1;
            }

            ObjectInstance* exception = vm->GetUnhandledException();
            if (exception == nullptr)
                return -1;

            *outException = exception;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMGetUnhandledExceptionMessage(VirtualMachine* vm, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            return CopyWString(vm->GetUnhandledExceptionMessage(), buffer, bufferLen);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMGetUnhandledExceptionStackTrace(VirtualMachine* vm, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            return CopyWString(vm->GetUnhandledExceptionStackTrace(), buffer, bufferLen);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMGetStackTrace(VirtualMachine* vm, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            return CopyWString(vm->GetStackTrace(), buffer, bufferLen);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API CallStackFrame* Shard_VMGetCurrentFrame(VirtualMachine* vm)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            return vm->CurrentFrame();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_VMRunInteractive(VirtualMachine* vm, std::size_t* pointer)
    {
        try
        {
            if (vm == nullptr || pointer == nullptr)
            {
                SetLastShardWError(L"virtual machine or pointer is null");
                return -1;
            }

            vm->RunInteractive(*pointer);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API CallStackFrame* Shard_VMPushFrame(VirtualMachine* vm, MethodSymbol* method)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            return vm->PushFrame(method);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_VMPopFrame(VirtualMachine* vm)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            vm->PopFrame();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_VMRaiseException(VirtualMachine* vm, ObjectInstance* exception)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return -1;
            }

            vm->RaiseException(exception);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_VMCreateRuntimeException(VirtualMachine* vm, const wchar_t* message, TypeSymbol* type)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            if (type == nullptr)
                type = SymbolTable::StandardTypes::RuntimeException;

            if (type == nullptr)
            {
                SetLastShardWError(L"RuntimeException type is not available");
                return nullptr;
            }

            std::wstring messageStr = message != nullptr ? message : L"";
            return vm->CreateRuntimeException(type, messageStr, vm->GetStackTrace());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetExceptionMessage(VirtualMachine* vm, ObjectInstance* exception)
    {
        try
        {
            if (vm == nullptr || exception == nullptr)
                return nullptr;

            static thread_local std::wstring buffer;
            buffer = vm->GetThrowablePropertyValue(exception, TRAIT_THROWABLE_getMessage);
            return buffer.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetExceptionStackTrace(VirtualMachine* vm, ObjectInstance* exception)
    {
        try
        {
            if (vm == nullptr || exception == nullptr)
                return nullptr;

            static thread_local std::wstring buffer;
            buffer = vm->GetThrowablePropertyValue(exception, TRAIT_THROWABLE_getStackTrace);
            return buffer.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_VMInstantiateObject(VirtualMachine* vm, TypeSymbol* type, ConstructorSymbol* ctor)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            if (type == nullptr || ctor == nullptr)
            {
                SetLastShardWError(L"type or constructor is null");
                return nullptr;
            }

            return vm->InstantiateObject(type, ctor);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_VMInstantiateDelegate(VirtualMachine* vm, DelegateTypeSymbol* type)
    {
        try
        {
            if (vm == nullptr)
            {
                SetLastShardWError(L"virtual machine is null");
                return nullptr;
            }

            if (type == nullptr)
            {
                SetLastShardWError(L"delegate type is null");
                return nullptr;
            }

            return vm->InstantiateDelegate(type);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Call Stack Frame API
    // =========================================================================

    SHARD_API std::size_t Shard_FrameEvalStackCount(CallStackFrame* frame)
    {
        try
        {
            if (frame == nullptr)
                return 0;

            return frame->EvalStack.size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_FramePushStack(CallStackFrame* frame, ObjectInstance* value)
    {
        try
        {
            if (frame == nullptr)
            {
                SetLastShardWError(L"frame is null");
                return -1;
            }

            frame->PushStack(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_FramePopStack(CallStackFrame* frame)
    {
        try
        {
            if (frame == nullptr)
            {
                SetLastShardWError(L"frame is null");
                return nullptr;
            }

            return frame->PopStack();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_FramePeekStack(CallStackFrame* frame)
    {
        try
        {
            if (frame == nullptr)
            {
                SetLastShardWError(L"frame is null");
                return nullptr;
            }

            return frame->PeekStack();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_FrameGetException(CallStackFrame* frame)
    {
        try
        {
            if (frame == nullptr)
            {
                SetLastShardWError(L"frame is null");
                return nullptr;
            }

            return frame->CurrentException;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_FrameGetInterruptionReason(CallStackFrame* frame)
    {
        try
        {
            if (frame == nullptr)
            {
                SetLastShardWError(L"frame is null");
                return -1;
            }

            return static_cast<int>(frame->InterruptionReason);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Event Loop API
    // =========================================================================

    SHARD_API int Shard_EventLoopRun(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetEventLoop().Run();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_EventLoopRunOnce(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetEventLoop().RunOnce();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_EventLoopStop(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetEventLoop().Stop();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_EventLoopIsAlive(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
                return 0;

            return domain->GetEventLoop().IsAlive() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_EventLoopRootTask(ApplicationDomain* domain, ObjectInstance* task)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetEventLoop().RootTask(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_EventLoopUnrootTask(ApplicationDomain* domain, ObjectInstance* task)
    {
        try
        {
            if (domain == nullptr)
            {
                SetLastShardWError(L"domain is null");
                return -1;
            }

            domain->GetEventLoop().UnrootTask(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_EventLoopIsEmptyOrAllTasksCompleted(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
                return 1;

            return domain->GetEventLoop().IsEmptyOrAllTasksCompleted() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetTaskState(ObjectInstance* task, FieldSymbol* stateField, int* state)
    {
        try
        {
            if (task == nullptr || stateField == nullptr || state == nullptr)
            {
                SetLastShardWError(L"task, state field or output pointer is null");
                return -1;
            }

            AsyncState asyncState = GetTaskState(task, stateField);
            switch (asyncState)
            {
                case AsyncState::PENDING:
                    *state = 0;
                    break;
                case AsyncState::COMPLETED:
                    *state = 1;
                    break;
                case AsyncState::FAULTED:
                    *state = 2;
                    break;
                default:
                    *state = -1;
                    return -1;
            }

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ResumeContinuation(ApplicationDomain* domain, ObjectInstance* task, FieldSymbol* continuationField, MethodSymbol* moveNextMethod)
    {
        try
        {
            if (domain == nullptr || task == nullptr || continuationField == nullptr || moveNextMethod == nullptr)
            {
                SetLastShardWError(L"domain, task, continuation field or move next method is null");
                return -1;
            }

            ResumeContinuation(task, continuationField, moveNextMethod, *domain);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Async Task API (C bindings for NativeAsync.hpp helpers)
    // =========================================================================

    SHARD_API ObjectInstance* Shard_DoAsync(const CallState* ctx, Shard_AsyncWorkCallback callback, void* userData)
    {
        try
        {
            if (ctx == nullptr || callback == nullptr)
            {
                SetLastShardWError(L"context or callback is null");
                return nullptr;
            }

            auto state = detail::CreateAsyncScopeState(*ctx, nullptr);
            ObjectInstance* task = state->task;
            RegisterAsyncState(task, state);

            AsyncScope scope(state);
            callback(task, userData);

            if (state->completed)
                UnregisterAsyncState(task);

            return task;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_DoValueTask(const CallState* ctx, TypeSymbol* resultType, Shard_AsyncWorkCallback callback, void* userData)
    {
        try
        {
            if (ctx == nullptr || callback == nullptr)
            {
                SetLastShardWError(L"context or callback is null");
                return nullptr;
            }

            auto state = detail::CreateAsyncScopeState(*ctx, resultType);
            ObjectInstance* task = state->task;
            RegisterAsyncState(task, state);

            AsyncScope scope(state);
            callback(task, userData);

            if (state->completed)
                UnregisterAsyncState(task);

            return task;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_CompletedTask(const CallState* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"context is null");
                return nullptr;
            }

            return shard::CompletedTask(*ctx);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_FaultedTask(const CallState* ctx, ObjectInstance* exception)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"context is null");
                return nullptr;
            }

            return shard::FaultedTask(*ctx, exception);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_FaultedTaskWithMessage(const CallState* ctx, const wchar_t* message)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"context is null");
                return nullptr;
            }

            return shard::FaultedTask(*ctx, message != nullptr ? std::wstring(message) : std::wstring());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_TaskComplete(Shard_AsyncScopeHandle task)
    {
        try
        {
            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            if (!state->completed)
            {
                if (state->isValueTask)
                    state->SetValueTaskResult(GarbageCollector::NullInstance);
                else
                    state->CompleteTask();
            }

            UnregisterAsyncState(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskFail(Shard_AsyncScopeHandle task, ObjectInstance* exception)
    {
        try
        {
            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            if (!state->completed)
            {
                if (exception == nullptr)
                    exception = GarbageCollector::NullInstance;

                if (state->isValueTask)
                    state->FailValueTask(exception);
                else
                    state->FailTask(exception);
            }

            UnregisterAsyncState(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskFailWithMessage(Shard_AsyncScopeHandle task, const wchar_t* message)
    {
        try
        {
            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            if (!state->completed)
            {
                ObjectInstance* exception = CreateRuntimeException(
                    *state->collector,
                    message != nullptr ? std::wstring(message) : std::wstring());

                if (state->isValueTask)
                    state->FailValueTask(exception);
                else
                    state->FailTask(exception);
            }

            UnregisterAsyncState(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskSetValueTaskResult(Shard_AsyncScopeHandle task, ObjectInstance* result)
    {
        try
        {
            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            if (!state->completed)
            {
                if (result == nullptr)
                    result = GarbageCollector::NullInstance;

                state->SetValueTaskResult(result);
            }

            UnregisterAsyncState(task);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskDelay(Shard_AsyncScopeHandle task, std::int64_t milliseconds, Shard_AsyncCallback callback, void* userData)
    {
        try
        {
            if (callback == nullptr)
            {
                SetLastShardWError(L"callback is null");
                return -1;
            }

            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            AsyncScope scope(state);
            scope.Delay(milliseconds, [callback, userData]() { callback(userData); });
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskRunOnThreadPool(Shard_AsyncScopeHandle task, Shard_AsyncCallback workCallback, void* workUserData, Shard_AsyncCallback completeCallback, void* completeUserData)
    {
        try
        {
            if (workCallback == nullptr || completeCallback == nullptr)
            {
                SetLastShardWError(L"work or complete callback is null");
                return -1;
            }

            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            AsyncScope scope(state);
            scope.RunOnThreadPool(
                [workCallback, workUserData]() { workCallback(workUserData); },
                [completeCallback, completeUserData]() { completeCallback(completeUserData); });
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskAwait(Shard_AsyncScopeHandle task, ObjectInstance* awaitable, Shard_AsyncCallback callback, void* userData)
    {
        try
        {
            if (callback == nullptr)
            {
                SetLastShardWError(L"callback is null");
                return -1;
            }

            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            AsyncScope scope(state);
            scope.Await(awaitable, [callback, userData]() { callback(userData); });
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TaskAwaitResult(Shard_AsyncScopeHandle task, ObjectInstance* awaitable, Shard_AsyncResultCallback callback, void* userData)
    {
        try
        {
            if (callback == nullptr)
            {
                SetLastShardWError(L"callback is null");
                return -1;
            }

            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return -1;

            AsyncScope scope(state);
            scope.AwaitResult(awaitable, [callback, userData](ObjectInstance* result) { callback(result, userData); });
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_CreateNativeContinuation(Shard_AsyncScopeHandle task, Shard_AsyncCallback callback, void* userData)
    {
        try
        {
            if (callback == nullptr)
            {
                SetLastShardWError(L"callback is null");
                return nullptr;
            }

            auto state = GetAsyncStateChecked(task);
            if (state == nullptr)
                return nullptr;

            return detail::CreateNativeContinuation(*state, [callback, userData]() { callback(userData); });
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_InvokeNativeContinuationCallback(ObjectInstance* continuation)
    {
        try
        {
            if (continuation == nullptr)
            {
                SetLastShardWError(L"continuation is null");
                return -1;
            }

            detail::InvokeNativeContinuationCallback(continuation);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetTaskState(ObjectInstance* task, FieldSymbol* stateField, int state, GarbageCollector* gc)
    {
        try
        {
            if (task == nullptr || stateField == nullptr)
            {
                SetLastShardWError(L"task or state field is null");
                return -1;
            }

            AsyncState asyncState;
            switch (state)
            {
                case 0:
                    asyncState = AsyncState::PENDING;
                    break;
                case 1:
                    asyncState = AsyncState::COMPLETED;
                    break;
                case 2:
                    asyncState = AsyncState::FAULTED;
                    break;
                default:
                    SetLastShardWError(L"invalid async state");
                    return -1;
            }

            GarbageCollector* collector = gc;
            if (collector == nullptr)
            {
                auto asyncStatePtr = GetAsyncState(task);
                if (asyncStatePtr == nullptr)
                {
                    SetLastShardWError(L"collector is null and task is not registered");
                    return -1;
                }
                collector = asyncStatePtr->collector;
            }

            shard::SetTaskState(task, stateField, asyncState, *collector);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API std::size_t Shard_EventLoopGetRootedTaskCount(ApplicationDomain* domain)
    {
        try
        {
            if (domain == nullptr)
                return 0;

            return domain->GetEventLoop().GetRootedTasks().size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ObjectInstance* Shard_EventLoopGetRootedTask(ApplicationDomain* domain, std::size_t index)
    {
        try
        {
            if (domain == nullptr)
                return nullptr;

            const auto& tasks = domain->GetEventLoop().GetRootedTasks();
            if (index >= tasks.size())
                return nullptr;

            return tasks[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Garbage Collector / Value API
    // =========================================================================

    SHARD_API ObjectInstance* Shard_GCFromInteger(GarbageCollector* gc, std::int64_t value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromDouble(GarbageCollector* gc, double value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromBool(GarbageCollector* gc, int value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromValue(value != 0);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromString(GarbageCollector* gc, const wchar_t* value)
    {
        try
        {
            if (gc == nullptr || value == nullptr)
            {
                SetLastShardWError(L"garbage collector or value is null");
                return nullptr;
            }

            return gc->FromValue(value, false);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromByte(GarbageCollector* gc, std::uint8_t value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromChar(GarbageCollector* gc, wchar_t value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromNint(GarbageCollector* gc, std::int64_t value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromNint(static_cast<std::intptr_t>(value), false);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromStringWithTransient(GarbageCollector* gc, const wchar_t* value, int isTransient)
    {
        try
        {
            if (gc == nullptr || value == nullptr)
            {
                SetLastShardWError(L"garbage collector or value is null");
                return nullptr;
            }

            return gc->FromValue(value, isTransient != 0);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromIntegerWithTransient(GarbageCollector* gc, std::int64_t value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            // NOTE: GarbageCollector::FromValue has no transient overload for integers;
            // the isTransient flag is accepted for API symmetry but currently ignored.
            (void)isTransient;
            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromDoubleWithTransient(GarbageCollector* gc, double value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            // NOTE: GarbageCollector::FromValue has no transient overload for doubles;
            // the isTransient flag is accepted for API symmetry but currently ignored.
            (void)isTransient;
            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromBoolWithTransient(GarbageCollector* gc, int value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            // NOTE: GarbageCollector::FromValue has no transient overload for booleans;
            // the isTransient flag is accepted for API symmetry but currently ignored.
            (void)isTransient;
            return gc->FromValue(value != 0);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromByteWithTransient(GarbageCollector* gc, std::uint8_t value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            // NOTE: GarbageCollector::FromValue has no transient overload for bytes;
            // the isTransient flag is accepted for API symmetry but currently ignored.
            (void)isTransient;
            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromCharWithTransient(GarbageCollector* gc, wchar_t value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            // NOTE: GarbageCollector::FromValue has no transient overload for characters;
            // the isTransient flag is accepted for API symmetry but currently ignored.
            (void)isTransient;
            return gc->FromValue(value);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromNintWithTransient(GarbageCollector* gc, std::int64_t value, int isTransient)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromNint(static_cast<std::intptr_t>(value), isTransient != 0);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCCopyInstance(GarbageCollector* gc, ObjectInstance* instance)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return nullptr;
            }

            return gc->CopyInstance(instance);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GCTerminateInstance(GarbageCollector* gc, ObjectInstance* instance)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return -1;
            }

            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            gc->TerminateInstance(instance, true);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_GCNullInstance(GarbageCollector* gc)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return GarbageCollector::NullInstance;
            }

            return GarbageCollector::NullInstance;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return GarbageCollector::NullInstance;
        }
    }

    SHARD_API int Shard_GCCollectInstance(GarbageCollector* gc, ObjectInstance* instance)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return -1;
            }

            gc->CollectInstance(instance);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GCDestroyInstance(GarbageCollector* gc, ObjectInstance* instance)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return -1;
            }

            gc->DestroyInstance(instance);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GCTerminate(GarbageCollector* gc)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return -1;
            }

            gc->Terminate();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API std::size_t Shard_GCGetHeapSize(GarbageCollector* gc)
    {
        try
        {
            if (gc == nullptr)
                return 0;

            return gc->Heap.size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ObjectInstance* Shard_GCFromNintPointer(GarbageCollector* gc, void* value)
    {
        try
        {
            if (gc == nullptr)
            {
                SetLastShardWError(L"garbage collector is null");
                return nullptr;
            }

            return gc->FromNint(value, false);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API std::int64_t Shard_ReadInteger(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->AsInteger();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API double Shard_ReadDouble(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0.0;

            return instance->AsDouble();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0.0;
        }
    }

    SHARD_API int Shard_ReadBool(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->AsBoolean() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API const wchar_t* Shard_ReadString(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return instance->AsString();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Object Instance API
    // =========================================================================

    SHARD_API TypeSymbol* Shard_GetObjectType(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return const_cast<TypeSymbol*>(instance->getInfo());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API std::size_t Shard_GetObjectArrayLength(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->GetArrayLength();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsNullInstance(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 1;

            return instance->IsNullInstance() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 1;
        }
    }

    SHARD_API int Shard_IsObjectInBounds(ObjectInstance* instance, std::size_t index)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->IsInBounds(index) ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ObjectInstance* Shard_GetObjectFieldBySlot(ObjectInstance* instance, std::uint32_t slot)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return instance->GetField(slot);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetObjectFieldBySlot(ObjectInstance* instance, std::uint32_t slot, ObjectInstance* value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->SetField(slot, value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteByte(ObjectInstance* instance, std::uint8_t value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->WriteByte(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteChar(ObjectInstance* instance, wchar_t value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->WriteCharacter(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteNint(ObjectInstance* instance, std::int64_t value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            void* ptr = reinterpret_cast<void*>(static_cast<intptr_t>(value));
            instance->WriteMemory(0, sizeof(void*), &ptr);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectAsByte(ObjectInstance* instance, std::uint8_t* out)
    {
        try
        {
            if (instance == nullptr || out == nullptr)
            {
                SetLastShardWError(L"instance or output pointer is null");
                return -1;
            }

            *out = instance->AsByte();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectAsChar(ObjectInstance* instance, wchar_t* out)
    {
        try
        {
            if (instance == nullptr || out == nullptr)
            {
                SetLastShardWError(L"instance or output pointer is null");
                return -1;
            }

            *out = instance->AsCharacter();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectAsNint(ObjectInstance* instance, std::int64_t* out)
    {
        try
        {
            if (instance == nullptr || out == nullptr)
            {
                SetLastShardWError(L"instance or output pointer is null");
                return -1;
            }

            *out = static_cast<std::int64_t>(reinterpret_cast<intptr_t>(instance->AsNint()));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectIncrementReference(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->IncrementReference();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectDecrementReference(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->DecrementReference();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API std::int64_t Shard_ObjectGetReferenceCount(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->getReferencesCounter();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_ObjectGetIsTransient(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->getIsTransient() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API void* Shard_ObjectGetMemory(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return instance->getMemory();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeShape* Shard_ObjectGetShape(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return instance->getShape();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_ObjectReadMemory(ObjectInstance* instance, std::size_t offset, std::size_t size, void* dst)
    {
        try
        {
            if (instance == nullptr || dst == nullptr)
            {
                SetLastShardWError(L"instance or destination pointer is null");
                return -1;
            }

            instance->ReadMemory(offset, size, dst);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteMemory(ObjectInstance* instance, std::size_t offset, std::size_t size, const void* src)
    {
        try
        {
            if (instance == nullptr || src == nullptr)
            {
                SetLastShardWError(L"instance or source pointer is null");
                return -1;
            }

            instance->WriteMemory(offset, size, src);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API void* Shard_ObjectOffsetMemory(ObjectInstance* instance, std::size_t offset, std::size_t size)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return nullptr;
            }

            return instance->OffsetMemory(offset, size);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_ObjectWriteInteger(ObjectInstance* instance, std::int64_t value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->WriteInteger(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteDouble(ObjectInstance* instance, double value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->WriteDouble(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteBool(ObjectInstance* instance, int value)
    {
        try
        {
            if (instance == nullptr)
            {
                SetLastShardWError(L"instance is null");
                return -1;
            }

            instance->WriteBoolean(value != 0);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ObjectWriteString(ObjectInstance* instance, const wchar_t* value)
    {
        try
        {
            if (instance == nullptr || value == nullptr)
            {
                SetLastShardWError(L"instance or value is null");
                return -1;
            }

            instance->WriteString(value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // TypeShape API
    // =========================================================================

    SHARD_API TypeSymbol* Shard_GetTypeShapeBaseType(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return nullptr;

            return shape->BaseType;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API std::size_t Shard_GetTypeShapeSize(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->Size;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API std::size_t Shard_GetTypeShapeSlotCount(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->Slots.size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API std::size_t Shard_GetTypeShapeSlotOffset(TypeShape* shape, std::uint32_t slot)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->GetOffset(slot);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeShape* Shard_GetTypeShapeSlotFieldShape(TypeShape* shape, std::uint32_t slot)
    {
        try
        {
            if (shape == nullptr)
                return nullptr;

            return shape->GetFieldShape(slot);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeShapeIsReferenceType(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->IsReferenceType() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetTypeShapeHasGenericArguments(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->HasGenericArguments() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API std::size_t Shard_GetTypeShapeGenericArgumentCount(TypeShape* shape)
    {
        try
        {
            if (shape == nullptr)
                return 0;

            return shape->GenericArguments.size();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeSymbol* Shard_GetTypeShapeGenericArgument(TypeShape* shape, std::size_t index)
    {
        try
        {
            if (shape == nullptr || index >= shape->GenericArguments.size())
                return nullptr;

            return shape->GenericArguments[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeShape* Shard_GetObjectTypeShape(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return nullptr;

            return instance->getShape();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeShape* Shard_GetOrCreateTypeShape(CompilationContext* ctx, TypeSymbol* baseType, TypeSymbol** genericArgs, std::size_t genericArgCount)
    {
        try
        {
            if (ctx == nullptr || baseType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            std::vector<TypeSymbol*> args;
            if (genericArgs != nullptr && genericArgCount > 0)
                args.assign(genericArgs, genericArgs + genericArgCount);

            return ctx->GetSemanticModel().TypeShapes->GetOrCreateShape(baseType, args);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeShape* Shard_GetTypeShapeForType(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return ctx->GetSemanticModel().TypeShapes->GetOrCreateShape(type);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Symbol Inspection API
    // =========================================================================

    SHARD_API int Shard_GetCompilationUnitCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            return static_cast<int>(ctx->GetSyntaxTree().CompilationUnits.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API CompilationUnitSyntax* Shard_GetCompilationUnit(CompilationContext* ctx, int index)
    {
        try
        {
            if (ctx == nullptr)
                return nullptr;

            auto& units = ctx->GetSyntaxTree().CompilationUnits;
            if (index < 0 || index >= static_cast<int>(units.size()))
                return nullptr;

            return units[index].get();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetCompilationUnitOrigin(CompilationUnitSyntax* unit)
    {
        try
        {
            if (unit == nullptr)
                return 0;

            return static_cast<int>(unit->Origin);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API NamespaceDeclarationSyntax* Shard_GetUnitNamespace(CompilationUnitSyntax* unit)
    {
        try
        {
            if (unit == nullptr)
                return nullptr;

            return unit->Namespace.get();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetNamespaceIdentifierCount(NamespaceDeclarationSyntax* ns)
    {
        try
        {
            if (ns == nullptr)
                return 0;

            return static_cast<int>(ns->IdentifierTokens.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API const wchar_t* Shard_GetNamespaceIdentifier(NamespaceDeclarationSyntax* ns, int index)
    {
        try
        {
            if (ns == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(ns->IdentifierTokens.size()))
                return nullptr;

            return ns->IdentifierTokens[index].Word.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetUnitClassCount(CompilationUnitSyntax* unit)
    {
        try
        {
            if (unit == nullptr)
                return 0;

            int count = 0;
            for (const auto& member : unit->Members)
            {
                if (member->Kind == SyntaxKind::ClassDeclaration)
                    count++;
            }

            return count;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ClassDeclarationSyntax* Shard_GetUnitClass(CompilationUnitSyntax* unit, int index)
    {
        try
        {
            if (unit == nullptr)
                return nullptr;

            int current = 0;
            for (const auto& member : unit->Members)
            {
                if (member->Kind == SyntaxKind::ClassDeclaration)
                {
                    if (current == index)
                        return static_cast<ClassDeclarationSyntax*>(member.get());

                    current++;
                }
            }

            return nullptr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetTypeName(TypeDeclarationSyntax* type)
    {
        try
        {
            if (type == nullptr)
                return nullptr;

            return type->IdentifierToken.Word.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeMethodCount(CompilationContext* ctx, TypeDeclarationSyntax* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return 0;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            return static_cast<int>(typeSymbol->Methods.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API MethodSymbol* Shard_GetTypeMethod(CompilationContext* ctx, TypeDeclarationSyntax* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return nullptr;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            if (index < 0 || index >= static_cast<int>(typeSymbol->Methods.size()))
                return nullptr;

            return typeSymbol->Methods[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetSymbolTableTypeCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            return static_cast<int>(ctx->GetSemanticModel().Table->GetTypeSymbols().size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeSymbol* Shard_GetSymbolTableType(CompilationContext* ctx, int index)
    {
        try
        {
            if (ctx == nullptr)
                return nullptr;

            auto types = ctx->GetSemanticModel().Table->GetTypeSymbols();
            if (index < 0 || index >= static_cast<int>(types.size()))
                return nullptr;

            return types[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeSymbol* Shard_FindType(CompilationContext* ctx, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
                return nullptr;

            return SemanticModel::FindTypeByName(&ctx->GetSemanticModel(), std::wstring(name));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_FindMethodInType(TypeSymbol* type, const wchar_t* name, int parameterCount)
    {
        try
        {
            if (type == nullptr || name == nullptr)
                return nullptr;

            for (MethodSymbol* method : type->Methods)
            {
                if (method != nullptr && method->Name == name &&
                    static_cast<int>(method->Parameters.size()) == parameterCount)
                {
                    return method;
                }
            }

            return nullptr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeSymbol* Shard_FindTypeByName(CompilationContext* ctx, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
                return nullptr;

            return SemanticModel::FindTypeByName(&ctx->GetSemanticModel(), std::wstring(name));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API FieldSymbol* Shard_FindFieldByName(CompilationContext* ctx, TypeSymbol* type, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || type == nullptr || name == nullptr)
                return nullptr;

            return SemanticModel::FindFieldByName(type, std::wstring(name));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_FindMethodByName(CompilationContext* ctx, TypeSymbol* type, const wchar_t* name, int parameterCount)
    {
        try
        {
            if (ctx == nullptr || type == nullptr || name == nullptr)
                return nullptr;

            for (MethodSymbol* method : type->Methods)
            {
                if (method != nullptr && method->Name == name &&
                    static_cast<int>(method->Parameters.size()) == parameterCount)
                {
                    return method;
                }
            }

            return nullptr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AreTypesEqual(TypeSymbol* a, TypeSymbol* b)
    {
        try
        {
            return SemanticModel::AreTypesEqual(a, b) ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsPrimitiveType(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            return SemanticModel::IsPrimitiveType(type) ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetTypeDisplayName(TypeSymbol* type, wchar_t* buffer, int bufferLen)
    {
        try
        {
            if (type == nullptr)
                return 0;

            return CopyWString(SemanticModel::GetTypeDisplayName(type), buffer, bufferLen);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetSymbolTableNamespaceCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            return static_cast<int>(ctx->GetSemanticModel().Table->GetNamespaceSymbols().size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API NamespaceSymbol* Shard_GetSymbolTableNamespace(CompilationContext* ctx, int index)
    {
        try
        {
            if (ctx == nullptr)
                return nullptr;

            auto namespaces = ctx->GetSemanticModel().Table->GetNamespaceSymbols();
            if (index < 0 || index >= static_cast<int>(namespaces.size()))
                return nullptr;

            return namespaces[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetSymbolTableMethodCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            return static_cast<int>(ctx->GetSemanticModel().Table->GetMethodSymbols().size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API MethodSymbol* Shard_GetSymbolTableMethod(CompilationContext* ctx, int index)
    {
        try
        {
            if (ctx == nullptr)
                return nullptr;

            auto methods = ctx->GetSemanticModel().Table->GetMethodSymbols();
            if (index < 0 || index >= static_cast<int>(methods.size()))
                return nullptr;

            return methods[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API SyntaxSymbol* Shard_LookupSymbol(CompilationContext* ctx, SyntaxNode* node)
    {
        try
        {
            if (ctx == nullptr || node == nullptr)
                return nullptr;

            auto result = ctx->GetSemanticModel().Table->LookupSymbol(node);
            return result.has_value() ? result.value() : nullptr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API SyntaxNode* Shard_LookupNode(CompilationContext* ctx, SyntaxSymbol* symbol)
    {
        try
        {
            if (ctx == nullptr || symbol == nullptr)
                return nullptr;

            auto result = ctx->GetSemanticModel().Table->LookupNode(symbol);
            return result.has_value() ? result.value() : nullptr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_MarkAllSymbolsReady(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            ctx->GetSemanticModel().Table->MarkAllSymbolsReady();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API const wchar_t* Shard_GetSymbolName(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return nullptr;

            return symbol->Name.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetMethodName(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return nullptr;

            return method->Name.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetMethodParameterCount(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->Parameters.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API const wchar_t* Shard_GetMethodParameterName(MethodSymbol* method, int index)
    {
        try
        {
            if (method == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(method->Parameters.size()))
                return nullptr;

            return method->Parameters[index]->Name.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeSymbol* Shard_GetMethodParameterType(MethodSymbol* method, int index)
    {
        try
        {
            if (method == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(method->Parameters.size()))
                return nullptr;

            return method->Parameters[index]->Type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeSymbol* Shard_GetMethodReturnType(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return nullptr;

            return method->ReturnType;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_IsMethodStatic(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return method->Linking == LINK_STATIC ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    // =========================================================================
    // Utility API
    // =========================================================================

    SHARD_API const wchar_t* Shard_GetVersion()
    {
        return SHARDSCRIPT_WVERSION;
    }

    SHARD_API int Shard_DestroySyntaxNode(SyntaxNode* node)
    {
        try
        {
            delete node;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_DestroySymbol(SyntaxSymbol* symbol)
    {
        try
        {
            delete symbol;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Library Metadata API
    // =========================================================================

    SHARD_API int Shard_ReadLibraryMetadata(const wchar_t* path, ShardLibMetadata* out)
    {
        try
        {
            if (path == nullptr)
            {
                SetLastShardWError(L"library path is null");
                return -1;
            }

            if (out == nullptr)
            {
                SetLastShardWError(L"output metadata pointer is null");
                return -1;
            }

            std::filesystem::path libraryPath(path);
            utilities::SharedLibrary library(libraryPath);
            using ShardLib_GetMetadata_Func = void(*)(ShardLibMetadata&);

            ShardLib_GetMetadata_Func getMetadata = library.GetFunction<ShardLib_GetMetadata_Func>("ShardLib_GetMetadata");
            if (getMetadata == nullptr)
            {
                SetLastShardWError(L"library does not export ShardLib_GetMetadata");
                return -1;
            }

            getMetadata(*out);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetLibraryDependencyCount(const ShardLibMetadata* metadata)
    {
        try
        {
            if (metadata == nullptr)
                return 0;

            return static_cast<int>(metadata->DependenciesLength);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API const wchar_t* Shard_GetLibraryDependencyName(const ShardLibMetadata* metadata, int index)
    {
        try
        {
            if (metadata == nullptr || index < 0 || static_cast<std::size_t>(index) >= metadata->DependenciesLength)
                return nullptr;

            return metadata->Dependencies[index].Name;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetLibraryDependencyVersionExpression(const ShardLibMetadata* metadata, int index)
    {
        try
        {
            if (metadata == nullptr || index < 0 || static_cast<std::size_t>(index) >= metadata->DependenciesLength)
                return nullptr;

            return metadata->Dependencies[index].VersionExpression;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetLibraryName(const ShardLibMetadata* metadata)
    {
        try
        {
            if (metadata == nullptr)
                return nullptr;

            return metadata->Name;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetLibraryDescription(const ShardLibMetadata* metadata)
    {
        try
        {
            if (metadata == nullptr)
                return nullptr;

            return metadata->Description;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API const wchar_t* Shard_GetLibraryVersion(const ShardLibMetadata* metadata)
    {
        try
        {
            if (metadata == nullptr)
                return nullptr;

            return metadata->Version;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Syntax Builder API
    // =========================================================================

    SHARD_API CompilationUnitSyntax* Shard_CreateCompilationUnit(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            return new CompilationUnitSyntax();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddCompilationUnit(CompilationContext* ctx, CompilationUnitSyntax* unit)
    {
        try
        {
            if (ctx == nullptr || unit == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            ctx->GetSyntaxTree().CompilationUnits.push_back(std::unique_ptr<CompilationUnitSyntax>(unit));
            ctx->MarkForReAnalyze();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_MarkForReAnalyze(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            ctx->MarkForReAnalyze();
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetCompilationUnitOrigin(CompilationUnitSyntax* unit, CompilationUnitOrigin origin)
    {
        try
        {
            if (unit == nullptr)
            {
                SetLastShardWError(L"compilation unit is null");
                return -1;
            }

            unit->Origin = origin;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetCompilationUnitNamespace(CompilationUnitSyntax* unit, NamespaceDeclarationSyntax* ns)
    {
        try
        {
            if (unit == nullptr)
            {
                SetLastShardWError(L"compilation unit is null");
                return -1;
            }

            unit->Namespace.reset(ns);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddCompilationUnitMember(CompilationUnitSyntax* unit, MemberDeclarationSyntax* member)
    {
        try
        {
            if (unit == nullptr || member == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            unit->Members.push_back(std::unique_ptr<MemberDeclarationSyntax>(member));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API NamespaceDeclarationSyntax* Shard_CreateNamespaceDeclaration(SyntaxNode* parent)
    {
        try
        {
            auto* ns = new NamespaceDeclarationSyntax(parent);
            ns->SemicolonToken = MakeToken(shard::TokenType::Semicolon, L";");
            return ns;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddNamespaceIdentifier(NamespaceDeclarationSyntax* ns, const wchar_t* name)
    {
        try
        {
            if (ns == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            ns->IdentifierTokens.push_back(MakeToken(shard::TokenType::Identifier, name));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddMemberModifier(MemberDeclarationSyntax* member, int modifierTokenType)
    {
        try
        {
            if (member == nullptr)
            {
                SetLastShardWError(L"member is null");
                return -1;
            }

            member->Modifiers.push_back(MakeToken(static_cast<shard::TokenType>(modifierTokenType)));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ClassDeclarationSyntax* Shard_CreateClassDeclaration(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            auto* decl = new ClassDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::ClassKeyword, L"class");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API StructDeclarationSyntax* Shard_CreateStructDeclaration(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            auto* decl = new StructDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::StructKeyword, L"struct");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddTypeMember(TypeDeclarationSyntax* type, MemberDeclarationSyntax* member)
    {
        try
        {
            if (type == nullptr || member == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            type->Members.push_back(std::unique_ptr<MemberDeclarationSyntax>(member));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API FieldDeclarationSyntax* Shard_CreateFieldDeclaration(SyntaxNode* parent, const wchar_t* name, TypeSyntax* type)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            auto* decl = new FieldDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::FieldKeyword, L"field");
            decl->ReturnType.reset(type);
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetFieldInitializer(FieldDeclarationSyntax* field, ExpressionSyntax* expression)
    {
        try
        {
            if (field == nullptr)
            {
                SetLastShardWError(L"field is null");
                return -1;
            }

            field->InitializerExpression.reset(expression);
            if (expression != nullptr)
                field->InitializerAssignToken = MakeToken(shard::TokenType::AssignOperator, L"=");

            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API MethodDeclarationSyntax* Shard_CreateMethodDeclaration(SyntaxNode* parent, const wchar_t* name, TypeSyntax* returnType)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            info.ReturnType.reset(returnType);
            auto* decl = new MethodDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::FunctionKeyword, L"func");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ConstructorDeclarationSyntax* Shard_CreateConstructorDeclaration(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            // The 'name' parameter is kept for binary compatibility but is ignored;
            // ShardScript constructors must always be named 'init'.
            (void)name;

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::InitKeyword, L"init");
            info.ReturnType = std::make_unique<PredefinedTypeSyntax>(MakeToken(shard::TokenType::VoidKeyword, L"void"), nullptr);
            auto* decl = new ConstructorDeclarationSyntax(info, parent);
            decl->DeclareToken = info.Identifier;
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetMethodReturnType(MethodDeclarationSyntax* method, TypeSyntax* returnType)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->ReturnType.reset(returnType);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetMethodParametersList(MethodDeclarationSyntax* method, ParametersListSyntax* parameters)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->ParametersList.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetMethodBody(MethodDeclarationSyntax* method, StatementsBlockSyntax* body)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->Body.reset(body);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetConstructorParametersList(ConstructorDeclarationSyntax* ctor, ParametersListSyntax* parameters)
    {
        try
        {
            if (ctor == nullptr)
            {
                SetLastShardWError(L"constructor is null");
                return -1;
            }

            ctor->ParametersList.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetConstructorBody(ConstructorDeclarationSyntax* ctor, StatementsBlockSyntax* body)
    {
        try
        {
            if (ctor == nullptr)
            {
                SetLastShardWError(L"constructor is null");
                return -1;
            }

            ctor->Body.reset(body);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API PropertyDeclarationSyntax* Shard_CreatePropertyDeclaration(SyntaxNode* parent, const wchar_t* name, TypeSyntax* type)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* prop = new PropertyDeclarationSyntax(parent);
            prop->IdentifierToken = MakeToken(shard::TokenType::Identifier, name);
            prop->ReturnType.reset(type);
            return prop;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_PropertyDeclarationAddGetter(PropertyDeclarationSyntax* property, AccessorDeclarationSyntax* getter)
    {
        try
        {
            if (property == nullptr)
            {
                SetLastShardWError(L"property is null");
                return -1;
            }

            property->Getter.reset(getter);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_PropertyDeclarationAddSetter(PropertyDeclarationSyntax* property, AccessorDeclarationSyntax* setter)
    {
        try
        {
            if (property == nullptr)
            {
                SetLastShardWError(L"property is null");
                return -1;
            }

            property->Setter.reset(setter);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API AccessorDeclarationSyntax* Shard_CreateAccessorDeclaration(SyntaxNode* parent, int keywordTokenType)
    {
        try
        {
            auto* accessor = new AccessorDeclarationSyntax(parent);
            accessor->KeywordToken = MakeToken(static_cast<shard::TokenType>(keywordTokenType));
            return accessor;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetAccessorBody(AccessorDeclarationSyntax* accessor, StatementsBlockSyntax* body)
    {
        try
        {
            if (accessor == nullptr)
            {
                SetLastShardWError(L"accessor is null");
                return -1;
            }

            accessor->Body.reset(body);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API InterfaceDeclarationSyntax* Shard_CreateInterfaceDeclaration(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            auto* decl = new InterfaceDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::InterfaceKeyword, L"interface");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API EnumDeclarationSyntax* Shard_CreateEnumDeclaration(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            auto* decl = new EnumDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::EnumKeyword, L"enum");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddEnumField(EnumDeclarationSyntax* enumDecl, EnumFieldDeclarationSyntax* field)
    {
        try
        {
            if (enumDecl == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            enumDecl->Fields.push_back(std::unique_ptr<EnumFieldDeclarationSyntax>(field));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API EnumFieldDeclarationSyntax* Shard_CreateEnumFieldDeclaration(SyntaxNode* parent, const wchar_t* name, ExpressionSyntax* value)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* field = new EnumFieldDeclarationSyntax(parent);
            field->IdentifierToken = MakeToken(shard::TokenType::Identifier, name);
            field->InitializerExpression.reset(value);
            if (value != nullptr)
                field->AssignToken = MakeToken(shard::TokenType::AssignOperator, L"=");
            return field;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API OperatorDeclarationSyntax* Shard_CreateOperatorDeclaration(SyntaxNode* parent, int operatorTokenType, TypeSyntax* returnType)
    {
        try
        {
            MemberDeclarationInfo info;
            info.ReturnType.reset(returnType);
            SyntaxToken operatorToken = MakeToken(static_cast<shard::TokenType>(operatorTokenType));
            auto* decl = new OperatorDeclarationSyntax(info, operatorToken, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::OperatorKeyword, L"operator");
            decl->OperatorToken = operatorToken;
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetOperatorParametersList(OperatorDeclarationSyntax* op, ParametersListSyntax* parameters)
    {
        try
        {
            if (op == nullptr)
            {
                SetLastShardWError(L"operator is null");
                return -1;
            }

            op->ParametersList.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetOperatorBody(OperatorDeclarationSyntax* op, StatementsBlockSyntax* body)
    {
        try
        {
            if (op == nullptr)
            {
                SetLastShardWError(L"operator is null");
                return -1;
            }

            op->Body.reset(body);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API IndexatorDeclarationSyntax* Shard_CreateIndexatorDeclaration(SyntaxNode* parent, TypeSyntax* returnType)
    {
        try
        {
            MemberDeclarationInfo info;
            info.ReturnType.reset(returnType);
            auto* decl = new IndexatorDeclarationSyntax(info, parent);
            decl->IndexKeyword = MakeToken(shard::TokenType::IndexerKeyword, L"indexer");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetIndexatorParametersList(IndexatorDeclarationSyntax* indexer, ParametersListSyntax* parameters)
    {
        try
        {
            if (indexer == nullptr)
            {
                SetLastShardWError(L"indexer is null");
                return -1;
            }

            indexer->ParametersList.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetIndexatorBody(IndexatorDeclarationSyntax* indexer, StatementsBlockSyntax* body)
    {
        try
        {
            if (indexer == nullptr)
            {
                SetLastShardWError(L"indexer is null");
                return -1;
            }

            // IndexatorDeclarationSyntax has Getter/Setter accessors rather than a direct Body.
            if (indexer->Getter == nullptr)
            {
                indexer->Getter = std::make_unique<AccessorDeclarationSyntax>(indexer);
                indexer->Getter->KeywordToken = MakeToken(shard::TokenType::GetKeyword, L"get");
            }

            indexer->Getter->Body.reset(body);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API DelegateDeclarationSyntax* Shard_CreateDelegateDeclaration(SyntaxNode* parent, const wchar_t* name, TypeSyntax* returnType)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            info.ReturnType.reset(returnType);
            auto* decl = new DelegateDeclarationSyntax(info, parent);
            decl->DelegateToken = MakeToken(shard::TokenType::DelegateKeyword, L"delegate");
            return decl;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetDelegateParametersList(DelegateDeclarationSyntax* delegate, ParametersListSyntax* parameters)
    {
        try
        {
            if (delegate == nullptr)
            {
                SetLastShardWError(L"delegate is null");
                return -1;
            }

            delegate->ParametersList.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API AttributeSyntax* Shard_CreateAttribute(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* attr = new AttributeSyntax(parent);
            attr->OpenBracketToken = MakeToken(shard::TokenType::OpenSquare, L"[");
            attr->CloseBracketToken = MakeToken(shard::TokenType::CloseSquare, L"]");
            attr->NameToken = MakeToken(shard::TokenType::Identifier, name);
            return attr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddAttribute(MemberDeclarationSyntax* member, AttributeSyntax* attribute)
    {
        try
        {
            if (member == nullptr || attribute == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            member->Attributes.push_back(std::unique_ptr<AttributeSyntax>(attribute));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API TypeParametersListSyntax* Shard_CreateTypeParametersList(SyntaxNode* parent)
    {
        try
        {
            return new TypeParametersListSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddTypeParameter(TypeParametersListSyntax* list, const wchar_t* name)
    {
        try
        {
            if (list == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            list->Types.push_back(MakeToken(shard::TokenType::Identifier, name));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetMethodTypeParametersList(MethodDeclarationSyntax* method, TypeParametersListSyntax* list)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->TypeParameters.reset(list);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetClassTypeParametersList(ClassDeclarationSyntax* cls, TypeParametersListSyntax* list)
    {
        try
        {
            if (cls == nullptr)
            {
                SetLastShardWError(L"class is null");
                return -1;
            }

            cls->TypeParameters.reset(list);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API WhereClauseSyntax* Shard_CreateWhereClause(SyntaxNode* parent, const wchar_t* typeParameterName)
    {
        try
        {
            if (typeParameterName == nullptr)
            {
                SetLastShardWError(L"type parameter name is null");
                return nullptr;
            }

            WhereClauseSyntax* whereClause = new WhereClauseSyntax(parent);
            whereClause->WhereKeywordToken = MakeToken(shard::TokenType::WhereKeyword);
            whereClause->TypeParameterToken = MakeToken(shard::TokenType::Identifier, typeParameterName);
            whereClause->ColonToken = MakeToken(shard::TokenType::Colon);
            return whereClause;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddWhereClauseConstraint(WhereClauseSyntax* whereClause, TypeSyntax* constraintType)
    {
        try
        {
            if (whereClause == nullptr || constraintType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            whereClause->ConstraintTypes.push_back(std::unique_ptr<TypeSyntax>(constraintType));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddMemberWhereClause(MemberDeclarationSyntax* member, WhereClauseSyntax* whereClause)
    {
        try
        {
            if (member == nullptr || whereClause == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            member->WhereClauses.push_back(std::unique_ptr<WhereClauseSyntax>(whereClause));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ParametersListSyntax* Shard_CreateParametersList(SyntaxNode* parent)
    {
        try
        {
            return new ParametersListSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddParameter(ParametersListSyntax* list, const wchar_t* name, TypeSyntax* type)
    {
        try
        {
            if (list == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            auto param = std::make_unique<ParameterSyntax>(std::unique_ptr<TypeSyntax>(type), MakeToken(shard::TokenType::Identifier, name), list);
            list->Parameters.push_back(std::move(param));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API StatementsBlockSyntax* Shard_CreateStatementsBlock(SyntaxNode* parent)
    {
        try
        {
            return new StatementsBlockSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddStatement(StatementsBlockSyntax* block, StatementSyntax* statement)
    {
        try
        {
            if (block == nullptr || statement == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            block->Statements.push_back(std::unique_ptr<StatementSyntax>(statement));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API PredefinedTypeSyntax* Shard_CreatePredefinedType(SyntaxNode* parent, int tokenType)
    {
        try
        {
            return new PredefinedTypeSyntax(MakeToken(static_cast<shard::TokenType>(tokenType)), parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IdentifierNameTypeSyntax* Shard_CreateIdentifierNameType(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* type = new IdentifierNameTypeSyntax(parent);
            type->Identifier = MakeToken(shard::TokenType::Identifier, name);
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ArrayTypeSyntax* Shard_CreateArrayType(SyntaxNode* parent, TypeSyntax* elementType, int rank)
    {
        try
        {
            auto* type = new ArrayTypeSyntax(std::unique_ptr<TypeSyntax>(elementType), parent);
            type->Rank = rank;
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API NullableTypeSyntax* Shard_CreateNullableType(SyntaxNode* parent, TypeSyntax* underlayingType)
    {
        try
        {
            auto* type = new NullableTypeSyntax(std::unique_ptr<TypeSyntax>(underlayingType), parent);
            type->QuestionToken = MakeToken(shard::TokenType::Question, L"?");
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API GenericTypeSyntax* Shard_CreateGenericType(SyntaxNode* parent, TypeSyntax* underlayingType)
    {
        try
        {
            auto* type = new GenericTypeSyntax(std::unique_ptr<TypeSyntax>(underlayingType), parent);
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeArgumentsListSyntax* Shard_CreateTypeArgumentsList(SyntaxNode* parent)
    {
        try
        {
            return new TypeArgumentsListSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddTypeArgument(TypeArgumentsListSyntax* list, TypeSyntax* type)
    {
        try
        {
            if (list == nullptr || type == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            list->Types.push_back(std::unique_ptr<TypeSyntax>(type));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetGenericTypeArguments(GenericTypeSyntax* generic, TypeArgumentsListSyntax* arguments)
    {
        try
        {
            if (generic == nullptr)
            {
                SetLastShardWError(L"generic type is null");
                return -1;
            }

            generic->Arguments.reset(arguments);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API QualifiedNameTypeSyntax* Shard_CreateQualifiedNameType(SyntaxNode* parent, TypeSyntax* left, const wchar_t* right)
    {
        try
        {
            if (right == nullptr)
            {
                SetLastShardWError(L"right is null");
                return nullptr;
            }

            auto* type = new QualifiedNameTypeSyntax(std::unique_ptr<TypeSyntax>(left), parent);
            type->Identifier = MakeToken(shard::TokenType::Identifier, right);
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API DelegateTypeSyntax* Shard_CreateDelegateType(SyntaxNode* parent, TypeSyntax* returnType)
    {
        try
        {
            auto* type = new DelegateTypeSyntax(parent);
            type->DelegateToken = MakeToken(shard::TokenType::DelegateKeyword, L"delegate");
            type->ReturnType.reset(returnType);
            return type;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetDelegateTypeParametersList(DelegateTypeSyntax* type, ParametersListSyntax* parameters)
    {
        try
        {
            if (type == nullptr)
            {
                SetLastShardWError(L"delegate type is null");
                return -1;
            }

            type->Params.reset(parameters);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API VariableStatementSyntax* Shard_CreateVariableStatement(SyntaxNode* parent, const wchar_t* name, TypeSyntax* type, ExpressionSyntax* initializer)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* stmt = new VariableStatementSyntax(
                std::unique_ptr<TypeSyntax>(type),
                MakeToken(shard::TokenType::Identifier, name),
                initializer != nullptr ? MakeToken(shard::TokenType::DeclareAssignOperator, L":=") : MakeToken(shard::TokenType::Unknown),
                std::unique_ptr<ExpressionSyntax>(initializer),
                parent);

            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ExpressionStatementSyntax* Shard_CreateExpressionStatement(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            if (expression == nullptr)
            {
                SetLastShardWError(L"expression is null");
                return nullptr;
            }

            return new ExpressionStatementSyntax(std::unique_ptr<ExpressionSyntax>(expression), parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ReturnStatementSyntax* Shard_CreateReturnStatement(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            auto* stmt = new ReturnStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ReturnKeyword, L"return");
            stmt->Expression.reset(expression);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ForEachStatementSyntax* Shard_CreateForEachStatement(SyntaxNode* parent, const wchar_t* variableName, ExpressionSyntax* range, StatementsBlockSyntax* body)
    {
        try
        {
            if (variableName == nullptr)
            {
                SetLastShardWError(L"variable name is null");
                return nullptr;
            }

            auto* stmt = new ForEachStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ForeachKeyword, L"foreach");
            stmt->IdentifierToken = MakeToken(shard::TokenType::Identifier, variableName);
            stmt->InKeywordToken = MakeToken(shard::TokenType::InKeyword, L"in");
            stmt->RangeExpression.reset(range);
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API WhileStatementSyntax* Shard_CreateWhileStatement(SyntaxNode* parent, ExpressionSyntax* condition, StatementsBlockSyntax* body)
    {
        try
        {
            auto* stmt = new WhileStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::WhileKeyword, L"while");
            stmt->ConditionExpression.reset(condition);
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ForStatementSyntax* Shard_CreateForStatement(SyntaxNode* parent, StatementSyntax* init, ExpressionSyntax* condition, StatementSyntax* after, StatementsBlockSyntax* body)
    {
        try
        {
            auto* stmt = new ForStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ForKeyword, L"for");
            stmt->InitializerStatement.reset(init);
            stmt->ConditionExpression.reset(condition);
            stmt->AfterRepeatStatement.reset(after);
            stmt->StatementsBlock.reset(body);
            stmt->FirstSemicolon = MakeToken(shard::TokenType::Semicolon, L";");
            stmt->SecondSemicolon = MakeToken(shard::TokenType::Semicolon, L";");
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ForInStatementSyntax* Shard_CreateForInStatement(SyntaxNode* parent, const wchar_t* variableName, ExpressionSyntax* range, StatementsBlockSyntax* body)
    {
        try
        {
            if (variableName == nullptr)
            {
                SetLastShardWError(L"variable name is null");
                return nullptr;
            }

            auto* stmt = new ForInStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ForKeyword, L"for");
            stmt->IdentifierToken = MakeToken(shard::TokenType::Identifier, variableName);
            stmt->InKeywordToken = MakeToken(shard::TokenType::InKeyword, L"in");
            stmt->RangeExpression.reset(range);
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API UntilStatementSyntax* Shard_CreateUntilStatement(SyntaxNode* parent, ExpressionSyntax* condition, StatementsBlockSyntax* body)
    {
        try
        {
            auto* stmt = new UntilStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::UntilKeyword, L"until");
            stmt->ConditionExpression.reset(condition);
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IfStatementSyntax* Shard_CreateIfStatement(SyntaxNode* parent, ExpressionSyntax* condition, StatementsBlockSyntax* thenBody)
    {
        try
        {
            auto* stmt = new IfStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::IfKeyword, L"if");
            if (condition != nullptr)
                stmt->ConditionExpression.reset(new ExpressionStatementSyntax(std::unique_ptr<ExpressionSyntax>(condition), stmt));
            stmt->StatementsBlock.reset(thenBody);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_IfStatementSetElse(IfStatementSyntax* ifStmt, ElseStatementSyntax* elseBody)
    {
        try
        {
            if (ifStmt == nullptr)
            {
                SetLastShardWError(L"if statement is null");
                return -1;
            }

            ifStmt->NextStatement.reset(elseBody);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API UnlessStatementSyntax* Shard_CreateUnlessStatement(SyntaxNode* parent, ExpressionSyntax* condition, StatementsBlockSyntax* body)
    {
        try
        {
            auto* stmt = new UnlessStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::UnlessKeyword, L"unless");
            if (condition != nullptr)
                stmt->ConditionExpression.reset(new ExpressionStatementSyntax(std::unique_ptr<ExpressionSyntax>(condition), stmt));
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ElseStatementSyntax* Shard_CreateElseStatement(SyntaxNode* parent, StatementsBlockSyntax* body)
    {
        try
        {
            auto* stmt = new ElseStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ElseKeyword, L"else");
            stmt->StatementsBlock.reset(body);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API SwitchStatementSyntax* Shard_CreateSwitchStatement(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            auto* stmt = new SwitchStatementSyntax(parent);
            stmt->SwitchKeywordToken = MakeToken(shard::TokenType::SwitchKeyword, L"switch");
            stmt->Expression.reset(expression);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddSwitchCase(SwitchStatementSyntax* switchStmt, SwitchCaseClauseSyntax* clause)
    {
        try
        {
            if (switchStmt == nullptr || clause == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            switchStmt->Clauses.push_back(std::unique_ptr<SwitchCaseClauseSyntax>(clause));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API SwitchCaseClauseSyntax* Shard_CreateSwitchCaseClause(SyntaxNode* parent, ExpressionSyntax* pattern, StatementsBlockSyntax* body)
    {
        try
        {
            auto* clause = new SwitchCaseClauseSyntax(parent);
            clause->KeywordToken = MakeToken(shard::TokenType::CaseKeyword, L"case");
            clause->ColonToken = MakeToken(shard::TokenType::Colon, L":");
            clause->Pattern.reset(pattern);
            clause->Body.reset(body);
            return clause;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ConditionalClauseSyntax* Shard_CreateConditionalClause(SyntaxNode* parent, ExpressionSyntax* condition, StatementSyntax* statement)
    {
        try
        {
            auto* clause = new ConditionalClauseSyntax(SyntaxKind::IfStatement, parent);
            clause->KeywordToken = MakeToken(shard::TokenType::IfKeyword, L"if");
            if (condition != nullptr)
                clause->ConditionExpression.reset(new ExpressionStatementSyntax(std::unique_ptr<ExpressionSyntax>(condition), clause));

            auto* block = dynamic_cast<StatementsBlockSyntax*>(statement);
            if (block != nullptr)
                clause->StatementsBlock.reset(block);

            return clause;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API LiteralExpressionSyntax* Shard_CreateLiteralExpression(SyntaxNode* parent, int tokenType, const wchar_t* value)
    {
        try
        {
            if (value == nullptr)
            {
                SetLastShardWError(L"value is null");
                return nullptr;
            }

            return new LiteralExpressionSyntax(MakeToken(static_cast<shard::TokenType>(tokenType), value), parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MemberAccessExpressionSyntax* Shard_CreateIdentifierExpression(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            return new MemberAccessExpressionSyntax(MakeToken(shard::TokenType::Identifier, name), std::unique_ptr<ExpressionSyntax>(nullptr), parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MemberAccessExpressionSyntax* Shard_CreateMemberAccessExpression(SyntaxNode* parent, ExpressionSyntax* previous, const wchar_t* memberName)
    {
        try
        {
            if (memberName == nullptr)
            {
                SetLastShardWError(L"member name is null");
                return nullptr;
            }

            auto* expr = new MemberAccessExpressionSyntax(MakeToken(shard::TokenType::Identifier, memberName), std::unique_ptr<ExpressionSyntax>(previous), parent);
            expr->DelimeterToken = MakeToken(shard::TokenType::Delimeter, L".");
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API BinaryExpressionSyntax* Shard_CreateBinaryExpression(SyntaxNode* parent, ExpressionSyntax* left, ExpressionSyntax* right, int operatorTokenType)
    {
        try
        {
            auto* expr = new BinaryExpressionSyntax(MakeToken(static_cast<shard::TokenType>(operatorTokenType)), parent);
            expr->Left.reset(left);
            expr->Right.reset(right);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API UnaryExpressionSyntax* Shard_CreateUnaryExpression(SyntaxNode* parent, ExpressionSyntax* operand, int operatorTokenType, int isPostfix)
    {
        try
        {
            auto* expr = new UnaryExpressionSyntax(MakeToken(static_cast<shard::TokenType>(operatorTokenType)), isPostfix != 0, parent);
            expr->Expression.reset(operand);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API InvokationExpressionSyntax* Shard_CreateInvocationExpression(SyntaxNode* parent, ExpressionSyntax* target, const wchar_t* methodName)
    {
        try
        {
            auto* expr = new InvokationExpressionSyntax(
                methodName != nullptr ? MakeToken(shard::TokenType::Identifier, methodName) : MakeToken(shard::TokenType::Unknown),
                std::unique_ptr<ExpressionSyntax>(target),
                parent);

            expr->DelimeterToken = MakeToken(shard::TokenType::Delimeter, L".");
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetInvocationArgumentsList(InvokationExpressionSyntax* invocation, ArgumentsListSyntax* arguments)
    {
        try
        {
            if (invocation == nullptr)
            {
                SetLastShardWError(L"invocation is null");
                return -1;
            }

            invocation->ArgumentsList.reset(arguments);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectExpressionSyntax* Shard_CreateObjectExpression(SyntaxNode* parent, TypeSyntax* type)
    {
        try
        {
            auto* expr = new ObjectExpressionSyntax(parent);
            expr->NewToken = MakeToken(shard::TokenType::NewKeyword, L"new");
            expr->Type.reset(type);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetObjectArgumentsList(ObjectExpressionSyntax* objectExpr, ArgumentsListSyntax* arguments)
    {
        try
        {
            if (objectExpr == nullptr)
            {
                SetLastShardWError(L"object expression is null");
                return -1;
            }

            objectExpr->ArgumentsList.reset(arguments);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API RangeExpressionSyntax* Shard_CreateRangeExpression(SyntaxNode* parent, ExpressionSyntax* left, ExpressionSyntax* right, int isInclusive)
    {
        try
        {
            auto* expr = new RangeExpressionSyntax(parent);
            expr->OperatorToken = MakeToken(isInclusive != 0 ? shard::TokenType::RangeInclusiveOperator : shard::TokenType::RangeOperator, isInclusive != 0 ? L"..&" : L"..");
            expr->Left.reset(left);
            expr->Right.reset(right);
            expr->IsInclusive = isInclusive != 0;
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API CollectionExpressionSyntax* Shard_CreateCollectionExpression(SyntaxNode* parent)
    {
        try
        {
            return new CollectionExpressionSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddCollectionElement(CollectionExpressionSyntax* collection, ExpressionSyntax* element)
    {
        try
        {
            if (collection == nullptr || element == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            collection->ValuesExpressions.push_back(std::unique_ptr<ExpressionSyntax>(element));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API TernaryExpressionSyntax* Shard_CreateTernaryExpression(SyntaxNode* parent, ExpressionSyntax* condition, ExpressionSyntax* trueExpr, ExpressionSyntax* falseExpr)
    {
        try
        {
            auto* expr = new TernaryExpressionSyntax(parent);
            expr->QuestionToken = MakeToken(shard::TokenType::Question, L"?");
            expr->ColonToken = MakeToken(shard::TokenType::Colon, L":");
            expr->Condition.reset(condition);
            expr->Left.reset(trueExpr);
            expr->Right.reset(falseExpr);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IfExpressionSyntax* Shard_CreateIfExpression(SyntaxNode* parent, ExpressionSyntax* condition, ExpressionSyntax* thenExpr, ExpressionSyntax* elseExpr)
    {
        try
        {
            auto* expr = new IfExpressionSyntax(parent);
            expr->IfKeywordToken = MakeToken(shard::TokenType::IfKeyword, L"if");
            expr->Condition.reset(condition);
            expr->ThenExpression.reset(thenExpr);
            expr->ElseExpression.reset(elseExpr);
            if (elseExpr != nullptr)
                expr->ElseKeywordToken = MakeToken(shard::TokenType::ElseKeyword, L"else");
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API SwitchExpressionSyntax* Shard_CreateSwitchExpression(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            auto* expr = new SwitchExpressionSyntax(parent);
            expr->SwitchKeywordToken = MakeToken(shard::TokenType::SwitchKeyword, L"switch");
            expr->Expression.reset(expression);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddSwitchExpressionArm(SwitchExpressionSyntax* switchExpr, SwitchArmSyntax* arm)
    {
        try
        {
            if (switchExpr == nullptr || arm == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            switchExpr->Arms.push_back(std::unique_ptr<SwitchArmSyntax>(arm));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API SwitchArmSyntax* Shard_CreateSwitchArm(SyntaxNode* parent, ExpressionSyntax* pattern, ExpressionSyntax* value)
    {
        try
        {
            auto* arm = new SwitchArmSyntax(parent);
            arm->ArrowToken = MakeToken(shard::TokenType::ArrowOperator, L"->");
            arm->Pattern.reset(pattern);
            arm->Expression.reset(value);
            return arm;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API CastExpressionSyntax* Shard_CreateCastExpression(SyntaxNode* parent, ExpressionSyntax* expression, TypeSyntax* targetType)
    {
        try
        {
            auto* expr = new CastExpressionSyntax(MakeToken(shard::TokenType::AsOperator, L"as"), parent);
            expr->Expression.reset(expression);
            expr->TargetType.reset(targetType);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IsExpressionSyntax* Shard_CreateIsExpression(SyntaxNode* parent, ExpressionSyntax* expression, TypeSyntax* type)
    {
        try
        {
            auto* expr = new IsExpressionSyntax(MakeToken(shard::TokenType::IsOperator, L"is"), parent);
            expr->Expression.reset(expression);
            expr->TargetType.reset(type);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IsPatternSyntax* Shard_CreateIsPattern(SyntaxNode* parent, TypeSyntax* type)
    {
        try
        {
            auto* expr = new IsPatternSyntax(MakeToken(shard::TokenType::IsOperator, L"is"), parent);
            expr->TargetType.reset(type);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IndexatorExpressionSyntax* Shard_CreateIndexatorExpression(SyntaxNode* parent, ExpressionSyntax* target, ExpressionSyntax* index)
    {
        try
        {
            SyntaxToken token = MakeToken(shard::TokenType::Unknown);
            auto* expr = new IndexatorExpressionSyntax(token, std::unique_ptr<ExpressionSyntax>(target), parent);
            auto* list = new IndexatorListSyntax(expr);
            list->OpenSquareToken = MakeToken(shard::TokenType::OpenSquare, L"[");
            list->CloseSquareToken = MakeToken(shard::TokenType::CloseSquare, L"]");
            if (index != nullptr)
                list->Arguments.push_back(std::make_unique<ArgumentSyntax>(std::unique_ptr<ExpressionSyntax>(index), list));
            expr->IndexatorList.reset(list);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API LambdaExpressionSyntax* Shard_CreateLambdaExpression(SyntaxNode* parent, ParametersListSyntax* parameters, TypeSyntax* returnType, StatementsBlockSyntax* body)
    {
        try
        {
            auto* expr = new LambdaExpressionSyntax(parent);
            expr->LambdaToken = MakeToken(shard::TokenType::LambdaKeyword, L"lambda");
            expr->LambdaOperatorToken = MakeToken(shard::TokenType::LambdaOperator, L"=>");
            expr->ParametersList.reset(parameters);
            expr->ReturnType.reset(returnType);
            expr->Body.reset(body);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API AwaitExpressionSyntax* Shard_CreateAwaitExpression(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            auto* expr = new AwaitExpressionSyntax(MakeToken(shard::TokenType::AwaitKeyword, L"await"), parent);
            expr->Expression.reset(expression);
            return expr;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeExpressionSyntax* Shard_CreateTypeExpression(SyntaxNode* parent, TypeSyntax* type)
    {
        try
        {
            return new TypeExpressionSyntax(std::unique_ptr<TypeSyntax>(type), parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ArgumentsListSyntax* Shard_CreateArgumentsList(SyntaxNode* parent)
    {
        try
        {
            return new ArgumentsListSyntax(parent);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddArgument(ArgumentsListSyntax* list, ExpressionSyntax* expression)
    {
        try
        {
            if (list == nullptr || expression == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            list->Arguments.push_back(std::make_unique<ArgumentSyntax>(std::unique_ptr<ExpressionSyntax>(expression), list));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_AddCompilationUnitUsing(CompilationUnitSyntax* unit, UsingDirectiveSyntax* usingDirective)
    {
        try
        {
            if (unit == nullptr || usingDirective == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            unit->Usings.push_back(std::unique_ptr<UsingDirectiveSyntax>(usingDirective));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API UsingDirectiveSyntax* Shard_CreateUsingDirective(SyntaxNode* parent, const wchar_t* name)
    {
        try
        {
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            auto* dir = new UsingDirectiveSyntax(parent);
            dir->UsingKeywordToken = MakeToken(shard::TokenType::UsingKeyword, L"using");
            dir->SemicolonToken = MakeToken(shard::TokenType::Semicolon, L";");
            dir->TokensList.push_back(MakeToken(shard::TokenType::Identifier, name));
            return dir;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddTypeBaseInterface(TypeDeclarationSyntax* type, TypeSyntax* interfaceType)
    {
        try
        {
            if (type == nullptr || interfaceType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            type->BaseInterfaces.push_back(std::unique_ptr<TypeSyntax>(interfaceType));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetStatementsBlockExpressionBody(StatementsBlockSyntax* block, int isExpressionBody)
    {
        try
        {
            if (block == nullptr)
            {
                SetLastShardWError(L"block is null");
                return -1;
            }

            block->IsExpressionBody = isExpressionBody != 0;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Symbol Builder API
    // =========================================================================

    SHARD_API SymbolTable* Shard_GetSymbolTable(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            return ctx->GetSemanticModel().Table.get();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeSymbol* Shard_GetPrimitiveType(CompilationContext* ctx, int primitiveKind)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            switch (primitiveKind)
            {
                case 0: return SymbolTable::Primitives::Void;
                case 1: return SymbolTable::Primitives::Null;
                case 2: return SymbolTable::Primitives::Any;
                case 3: return SymbolTable::Primitives::Boolean;
                case 4: return SymbolTable::Primitives::Integer;
                case 5: return SymbolTable::Primitives::Double;
                case 6: return SymbolTable::Primitives::Char;
                case 7: return SymbolTable::Primitives::String;
                case 8: return SymbolTable::Primitives::Array;
                case 9: return SymbolTable::Primitives::NativeInteger;
                case 10: return SymbolTable::Primitives::Byte;
                default:
                    SetLastShardWError(L"invalid primitive type kind");
                    return nullptr;
            }
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API NamespaceSymbol* Shard_CreateNamespaceSymbol(CompilationContext* ctx, NamespaceSymbol* parent, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Namespace(name);
            symbol->Parent = parent;
            symbol->Accesibility = SymbolAccesibility::Public;

            NamespaceNode* parentNode = parent != nullptr ? parent->Node : ctx->GetSemanticModel().Namespaces->Root;
            if (parentNode != nullptr)
                symbol->Node = parentNode->LookupOrCreate(symbol->Name, symbol);

            if (parent != nullptr)
            {
                parent->OnSymbolDeclared(symbol);
                symbol->FullName = parent->FullName + L"." + symbol->Name;
            }
            else
            {
                symbol->FullName = symbol->Name;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ClassSymbol* Shard_CreateClassSymbol(CompilationContext* ctx, NamespaceSymbol* parent, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Class(name);
            symbol->Parent = parent;
            symbol->Accesibility = SymbolAccesibility::Public;
            symbol->Inlining = TypeInlining::ByReference;

            if (parent != nullptr)
            {
                parent->OnSymbolDeclared(symbol);
                symbol->FullName = parent->FullName + L"." + symbol->Name;
            }
            else
            {
                symbol->FullName = symbol->Name;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_CreateMethodSymbol(CompilationContext* ctx, TypeSymbol* parentType, const wchar_t* name, TypeSymbol* returnType, int isStatic, int accessibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || parentType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Method(name, returnType, isStatic ? LINK_STATIC : LINK_INSTANCE);
            symbol->Parent = parentType;
            symbol->ReturnType = returnType;
            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);
            symbol->HandleType = MethodHandleType::External;

            parentType->OnSymbolDeclared(symbol);

            NamespaceSymbol* ns = parentType->Parent != nullptr && parentType->Parent->Kind == SyntaxKind::NamespaceDeclaration
                ? static_cast<NamespaceSymbol*>(parentType->Parent)
                : nullptr;

            if (ns != nullptr)
                symbol->FullName = ns->FullName + L"." + parentType->Name + L"." + symbol->Name;
            else
                symbol->FullName = parentType->Name + L"." + symbol->Name;

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_CreateNamespaceMethodSymbol(CompilationContext* ctx, NamespaceSymbol* parentNamespace, const wchar_t* name, TypeSymbol* returnType, int isStatic, int accessibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || parentNamespace == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Method(name, returnType, isStatic ? LINK_STATIC : LINK_INSTANCE);
            symbol->Parent = parentNamespace;
            symbol->ReturnType = returnType;
            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);
            symbol->HandleType = MethodHandleType::External;

            parentNamespace->OnSymbolDeclared(symbol);
            symbol->FullName = parentNamespace->FullName + L"." + symbol->Name;

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ConstructorSymbol* Shard_CreateConstructorSymbol(CompilationContext* ctx, TypeSymbol* parentType, int accessibility)
    {
        try
        {
            if (ctx == nullptr || parentType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Constructor(parentType, static_cast<SymbolAccesibility>(accessibility));
            symbol->Parent = parentType;
            symbol->ReturnType = SymbolTable::Primitives::Void;
            symbol->HandleType = MethodHandleType::External;

            parentType->OnSymbolDeclared(symbol);

            NamespaceSymbol* ns = parentType->Parent != nullptr && parentType->Parent->Kind == SyntaxKind::NamespaceDeclaration
                ? static_cast<NamespaceSymbol*>(parentType->Parent)
                : nullptr;

            if (ns != nullptr)
                symbol->FullName = ns->FullName + L"." + parentType->Name + L".init";
            else
                symbol->FullName = parentType->Name + L".init";

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ParameterSymbol* Shard_CreateParameterSymbol(CompilationContext* ctx, const wchar_t* name, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || type == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Parameter(name);
            symbol->Type = type;
            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddMethodParameter(MethodSymbol* method, ParameterSymbol* parameter)
    {
        try
        {
            if (method == nullptr || parameter == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            parameter->Parent = method;
            method->Parameters.push_back(parameter);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API FieldSymbol* Shard_CreateFieldSymbol(CompilationContext* ctx, TypeSymbol* parentType, const wchar_t* name, TypeSymbol* type, int isStatic, int accesibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || parentType == nullptr || type == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            auto* symbol = factory.Field(name, type, static_cast<SymbolLinking>(isStatic));
            symbol->Parent = parentType;
            symbol->ReturnType = type;
            symbol->Accesibility = static_cast<SymbolAccesibility>(accesibility);

            parentType->OnSymbolDeclared(symbol);
            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetSymbolAccesibility(SyntaxSymbol* symbol, int accessibility)
    {
        try
        {
            if (symbol == nullptr)
            {
                SetLastShardWError(L"symbol is null");
                return -1;
            }

            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetSymbolLinking(SyntaxSymbol* symbol, int linking)
    {
        try
        {
            if (symbol == nullptr)
            {
                SetLastShardWError(L"symbol is null");
                return -1;
            }

            MemberSymbol* memberSymbol = dynamic_cast<MemberSymbol*>(symbol);
            if (memberSymbol == nullptr)
            {
                SetLastShardWError(L"symbol does not support linking");
                return -1;
            }

            memberSymbol->Linking = static_cast<SymbolLinking>(linking);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetMethodManagedCallback(MethodSymbol* method, ShardManagedMethodCallback callback, void* userData)
    {
        try
        {
            if (method == nullptr || callback == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            ManagedMethodCallbacks[method] = { callback, userData };
            method->FunctionPointer = &InvokeManagedMethodCallback;
            method->HandleType = MethodHandleType::External;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetMethodManagedCallStateCallback(MethodSymbol* method, ShardManagedCallStateCallback callback, void* userData)
    {
        try
        {
            if (method == nullptr || callback == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            ManagedCallStateCallbacks[method] = { callback, userData };
            method->FunctionPointer = &InvokeManagedCallStateCallback;
            method->HandleType = MethodHandleType::External;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Extended Syntax Builder API
    // =========================================================================

    SHARD_API DeferStatementSyntax* Shard_CreateDeferStatement(SyntaxNode* parent, StatementSyntax* statement)
    {
        try
        {
            auto* defer = new DeferStatementSyntax(MakeToken(shard::TokenType::DeferKeyword, L"defer"), parent);
            defer->Statement.reset(statement);
            return defer;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API BreakStatementSyntax* Shard_CreateBreakStatement(SyntaxNode* parent)
    {
        try
        {
            auto* stmt = new BreakStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::BreakKeyword, L"break");
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ContinueStatementSyntax* Shard_CreateContinueStatement(SyntaxNode* parent)
    {
        try
        {
            auto* stmt = new ContinueStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ContinueKeyword, L"continue");
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ThrowStatementSyntax* Shard_CreateThrowStatement(SyntaxNode* parent, ExpressionSyntax* expression)
    {
        try
        {
            auto* stmt = new ThrowStatementSyntax(parent);
            stmt->KeywordToken = MakeToken(shard::TokenType::ThrowKeyword, L"throw");
            stmt->Expression.reset(expression);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TryStatementSyntax* Shard_CreateTryStatement(SyntaxNode* parent, StatementsBlockSyntax* tryBlock)
    {
        try
        {
            auto* stmt = new TryStatementSyntax(parent);
            stmt->TryKeywordToken = MakeToken(shard::TokenType::TryKeyword, L"try");
            stmt->TryBlock.reset(tryBlock);
            return stmt;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddCatchClause(TryStatementSyntax* tryStmt, const wchar_t* variableName, TypeSyntax* exceptionType, StatementsBlockSyntax* body)
    {
        try
        {
            if (tryStmt == nullptr || variableName == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            auto* clause = new CatchClauseSyntax(tryStmt);
            clause->CatchKeywordToken = MakeToken(shard::TokenType::CatchKeyword, L"catch");
            clause->OpenParenToken = MakeToken(shard::TokenType::OpenCurl, L"(");
            clause->IdentifierToken = MakeToken(shard::TokenType::Identifier, variableName);
            clause->ColonToken = MakeToken(shard::TokenType::Colon, L":");
            clause->ExceptionType.reset(exceptionType);
            clause->CloseParenToken = MakeToken(shard::TokenType::CloseCurl, L")");
            clause->Body.reset(body);

            tryStmt->CatchClauses.push_back(std::unique_ptr<CatchClauseSyntax>(clause));
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Extended Diagnostics API
    // =========================================================================

    SHARD_API int Shard_GetWarningCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            const auto& diagnostics = ctx->GetDiagnosticsContext().Diagnostics;
            return static_cast<int>(std::count_if(diagnostics.begin(), diagnostics.end(),
                [](const Diagnostic& d) { return d.Severity == DiagnosticSeverity::Warning; }));
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetDiagnosticCount(CompilationContext* ctx)
    {
        try
        {
            if (ctx == nullptr)
                return 0;

            return static_cast<int>(ctx->GetDiagnosticsContext().Diagnostics.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    static const Diagnostic* GetDiagnosticAt(CompilationContext* ctx, int index)
    {
        if (ctx == nullptr)
            return nullptr;

        const auto& diagnostics = ctx->GetDiagnosticsContext().Diagnostics;
        if (index < 0 || index >= static_cast<int>(diagnostics.size()))
            return nullptr;

        return &diagnostics[index];
    }

    SHARD_API int Shard_GetDiagnosticSeverity(CompilationContext* ctx, int index)
    {
        try
        {
            const Diagnostic* diagnostic = GetDiagnosticAt(ctx, index);
            if (diagnostic == nullptr)
                return -1;

            switch (diagnostic->Severity)
            {
                case DiagnosticSeverity::Info:    return 0;
                case DiagnosticSeverity::Warning: return 1;
                case DiagnosticSeverity::Error:   return 2;
                default:                          return -1;
            }
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetDiagnosticLine(CompilationContext* ctx, int index)
    {
        try
        {
            const Diagnostic* diagnostic = GetDiagnosticAt(ctx, index);
            if (diagnostic == nullptr)
                return -1;

            return diagnostic->Location.Line;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetDiagnosticColumn(CompilationContext* ctx, int index)
    {
        try
        {
            const Diagnostic* diagnostic = GetDiagnosticAt(ctx, index);
            if (diagnostic == nullptr)
                return -1;

            return diagnostic->Location.Offset;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetDiagnosticLength(CompilationContext* ctx, int index)
    {
        try
        {
            const Diagnostic* diagnostic = GetDiagnosticAt(ctx, index);
            if (diagnostic == nullptr)
                return -1;

            return diagnostic->Location.Length;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetDiagnosticMessage(CompilationContext* ctx, int index, wchar_t* buffer, int bufferLen)
    {
        try
        {
            const Diagnostic* diagnostic = GetDiagnosticAt(ctx, index);
            if (diagnostic == nullptr)
                return 0;

            const std::wstring& message = diagnostic->Description;
            if (buffer != nullptr && bufferLen > 0)
            {
                std::size_t copyLen = (std::min)((std::size_t)bufferLen - 1, message.length());
                wcsncpy(buffer, message.c_str(), copyLen);
                buffer[copyLen] = L'\0';
            }

            return (int)message.length();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    static int Shard_ReportDiagnostic(CompilationContext* ctx, const wchar_t* message, int line, int offset, int length, DiagnosticSeverity severity)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return -1;
            }

            if (message == nullptr)
            {
                SetLastShardWError(L"diagnostic message is null");
                return -1;
            }

            TextLocation location(L"", line, offset, length);
            SyntaxToken token(TokenType::Unknown, std::wstring(), location, true);

            switch (severity)
            {
                case DiagnosticSeverity::Error:
                    ctx->GetDiagnosticsContext().ReportError(token, message);
                    return 0;
                case DiagnosticSeverity::Warning:
                    ctx->GetDiagnosticsContext().ReportWarning(token, message);
                    return 0;
                case DiagnosticSeverity::Info:
                    ctx->GetDiagnosticsContext().ReportInfo(token, message);
                    return 0;
                default:
                    SetLastShardWError(L"unknown diagnostic severity");
                    return -1;
            }
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ReportError(CompilationContext* ctx, const wchar_t* message, int line, int offset, int length)
    {
        return Shard_ReportDiagnostic(ctx, message, line, offset, length, DiagnosticSeverity::Error);
    }

    SHARD_API int Shard_ReportWarning(CompilationContext* ctx, const wchar_t* message, int line, int offset, int length)
    {
        return Shard_ReportDiagnostic(ctx, message, line, offset, length, DiagnosticSeverity::Warning);
    }

    SHARD_API int Shard_ReportInfo(CompilationContext* ctx, const wchar_t* message, int line, int offset, int length)
    {
        return Shard_ReportDiagnostic(ctx, message, line, offset, length, DiagnosticSeverity::Info);
    }

    // =========================================================================
    // Extended Symbol Inspection API
    // =========================================================================

    SHARD_API int Shard_GetTypeFieldCount(CompilationContext* ctx, TypeDeclarationSyntax* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return 0;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            return static_cast<int>(typeSymbol->Fields.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API FieldSymbol* Shard_GetTypeField(CompilationContext* ctx, TypeDeclarationSyntax* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return nullptr;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            if (index < 0 || index >= static_cast<int>(typeSymbol->Fields.size()))
                return nullptr;

            return typeSymbol->Fields[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeInterfaceCount(CompilationContext* ctx, TypeDeclarationSyntax* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return 0;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            return static_cast<int>(typeSymbol->Interfaces.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeSymbol* Shard_GetTypeInterface(CompilationContext* ctx, TypeDeclarationSyntax* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            auto symbolOpt = ctx->GetSemanticModel().Table->LookupSymbol(type);
            if (!symbolOpt.has_value())
                return nullptr;

            auto* typeSymbol = static_cast<TypeSymbol*>(symbolOpt.value());
            if (index < 0 || index >= static_cast<int>(typeSymbol->Interfaces.size()))
                return nullptr;

            return typeSymbol->Interfaces[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API InterfaceSymbol* Shard_GetStandardInterface(CompilationContext* ctx, int kind)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            switch (kind)
            {
                case 0: return TRAIT_PRINTABLE;
                case 1: return TRAIT_DISPOSABLE;
                case 2: return TRAIT_ENUMERABLE;
                case 3: return TRAIT_THROWABLE;
                case 4: return TRAIT_ASYNCSTATE;
                case 5: return TRAIT_AWAITABLE;
                case 6: return TRAIT_AWAITER;
                case 7: return TRAIT_ENUMERATOR;
                default:
                    SetLastShardWError(L"invalid standard interface kind");
                    return nullptr;
            }
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_IsTypeAssignableFrom(TypeSymbol* target, TypeSymbol* source)
    {
        try
        {
            if (target == nullptr || source == nullptr)
                return 0;

            return SemanticModel::IsAssignableTo(target, source) ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeSymbol* Shard_GetFieldType(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return nullptr;

            return field->ReturnType;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_IsFieldStatic(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return 0;

            return field->Linking == LINK_STATIC ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API const wchar_t* Shard_GetFieldName(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return nullptr;

            return field->Name.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API FieldSymbol* Shard_FindFieldInType(TypeSymbol* type, const wchar_t* name)
    {
        try
        {
            if (type == nullptr || name == nullptr)
                return nullptr;

            std::wstring fieldName(name);
            return type->FindField(fieldName);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Symbol Metadata API
    // =========================================================================

    SHARD_API const wchar_t* Shard_GetSymbolFullName(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return nullptr;

            return symbol->FullName.c_str();
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetSymbolKind(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return 0;

            return static_cast<int>(symbol->Kind);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API SyntaxSymbol* Shard_GetSymbolParent(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return nullptr;

            return symbol->Parent;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetSymbolAnalysisState(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return 0;

            return static_cast<int>(symbol->AnalysisState);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetSymbolLinking(SyntaxSymbol* symbol)
    {
        try
        {
            if (symbol == nullptr)
                return -1;

            MemberSymbol* member = dynamic_cast<MemberSymbol*>(symbol);
            if (member == nullptr)
                return -1;

            return member->Linking == SymbolLinking::Static ? 0 : 1;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetTypeConstructorCount(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            return static_cast<int>(type->Constructors.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API MethodSymbol* Shard_GetTypeConstructor(CompilationContext* ctx, TypeSymbol* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(type->Constructors.size()))
                return nullptr;

            return type->Constructors[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypePropertyCount(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            return static_cast<int>(type->Properties.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API PropertySymbol* Shard_GetTypeProperty(CompilationContext* ctx, TypeSymbol* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(type->Properties.size()))
                return nullptr;

            return type->Properties[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeIndexatorCount(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            return static_cast<int>(type->Indexators.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API IndexatorSymbol* Shard_GetTypeIndexator(CompilationContext* ctx, TypeSymbol* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(type->Indexators.size()))
                return nullptr;

            return type->Indexators[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeOperatorCount(CompilationContext* ctx, TypeSymbol* type)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return 0;

            return static_cast<int>(type->Operators.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API OperatorSymbol* Shard_GetTypeOperator(CompilationContext* ctx, TypeSymbol* type, int index)
    {
        try
        {
            if (ctx == nullptr || type == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(type->Operators.size()))
                return nullptr;

            return type->Operators[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetTypeMemorySize(TypeSymbol* type)
    {
        try
        {
            if (type == nullptr)
                return 0;

            return static_cast<int>(type->MemoryBytesSize);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsTypeReferenceType(TypeSymbol* type)
    {
        try
        {
            if (type == nullptr)
                return 0;

            return type->IsReferenceType() ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsTypeNullable(TypeSymbol* type)
    {
        try
        {
            if (type == nullptr)
                return 0;

            return type->IsNullable ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsMethodAbstract(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return method->IsAbstract ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_IsMethodAsync(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return method->IsAsync ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_SetMethodAsync(MethodSymbol* method, int isAsync)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->IsAsync = isAsync != 0;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetMethodTypeParameterCount(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->TypeParameters.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API TypeParameterSymbol* Shard_GetMethodTypeParameter(MethodSymbol* method, int index)
    {
        try
        {
            if (method == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(method->TypeParameters.size()))
                return nullptr;

            return method->TypeParameters[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetMethodEvalStackArgumentsCount(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->GetEvalStackArgumentsCount());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetMethodEvalStackVariablesCount(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->GetEvalStackVariablesCount());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_GetMethodEvalStackLocalsCount(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->GetEvalStackLocalsCount());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API AccessorSymbol* Shard_GetPropertyGetter(PropertySymbol* property)
    {
        try
        {
            if (property == nullptr)
                return nullptr;

            return property->Getter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API AccessorSymbol* Shard_GetPropertySetter(PropertySymbol* property)
    {
        try
        {
            if (property == nullptr)
                return nullptr;

            return property->Setter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API FieldSymbol* Shard_GetPropertyBackingField(PropertySymbol* property)
    {
        try
        {
            if (property == nullptr)
                return nullptr;

            return property->BackingField;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetIndexatorParameterCount(IndexatorSymbol* indexator)
    {
        try
        {
            if (indexator == nullptr)
                return 0;

            return static_cast<int>(indexator->Parameters.size());
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ParameterSymbol* Shard_GetIndexatorParameter(IndexatorSymbol* indexator, int index)
    {
        try
        {
            if (indexator == nullptr)
                return nullptr;

            if (index < 0 || index >= static_cast<int>(indexator->Parameters.size()))
                return nullptr;

            return indexator->Parameters[index];
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_IsParameterOptional(ParameterSymbol* parameter)
    {
        try
        {
            if (parameter == nullptr)
                return 0;

            return parameter->IsOptional ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API ExpressionSyntax* Shard_GetParameterDefaultValue(ParameterSymbol* parameter)
    {
        try
        {
            if (parameter == nullptr)
                return nullptr;

            return parameter->DefaultValueExpression;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GetParameterSlotIndex(ParameterSymbol* parameter)
    {
        try
        {
            if (parameter == nullptr)
                return -1;

            return static_cast<int>(parameter->SlotIndex);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetFieldOffset(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return -1;

            return static_cast<int>(field->MemoryBytesOffset);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetFieldSlotIndex(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return -1;

            return static_cast<int>(field->SlotIndex);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_IsFieldEnumValue(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return 0;

            return field->IsEnumValue ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API std::int64_t Shard_GetFieldEnumValue(FieldSymbol* field)
    {
        try
        {
            if (field == nullptr)
                return 0;

            return field->EnumValue;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    // =========================================================================
    // Runtime Field Access API
    // =========================================================================

    SHARD_API ObjectInstance* Shard_GCGetStaticField(GarbageCollector* gc, FieldSymbol* field)
    {
        try
        {
            if (gc == nullptr || field == nullptr)
                return nullptr;

            return gc->GetStaticField(field);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_GCSetStaticField(GarbageCollector* gc, FieldSymbol* field, ObjectInstance* value)
    {
        try
        {
            if (gc == nullptr || field == nullptr)
                return -1;

            gc->SetStaticField(field, value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // Runtime Object Allocation API
    // =========================================================================

    SHARD_API ObjectInstance* Shard_GCAllocateInstance(GarbageCollector* gc, TypeSymbol* type)
    {
        try
        {
            if (gc == nullptr || type == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return gc->AllocateInstance(type);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCAllocateArray(GarbageCollector* gc, TypeSymbol* elementType, std::size_t length)
    {
        try
        {
            if (gc == nullptr || elementType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return gc->AllocateArray(elementType, length);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCAllocateInstanceFromShape(GarbageCollector* gc, TypeShape* shape)
    {
        try
        {
            if (gc == nullptr || shape == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return gc->AllocateInstance(shape);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API ObjectInstance* Shard_GCAllocateGeneric(GarbageCollector* gc, TypeSymbol* baseType, TypeSymbol** genericArgs, std::size_t genericArgCount)
    {
        try
        {
            if (gc == nullptr || baseType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            std::vector<TypeSymbol*> args;
            if (genericArgs != nullptr && genericArgCount > 0)
                args.assign(genericArgs, genericArgs + genericArgCount);

            return gc->AllocateGeneric(baseType, args);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // Runtime Instance Field / Element Access API
    // =========================================================================

    SHARD_API ObjectInstance* Shard_GetInstanceField(ObjectInstance* instance, FieldSymbol* field)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return instance->GetField(field->SlotIndex);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetInstanceField(ObjectInstance* instance, FieldSymbol* field, ObjectInstance* value)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            instance->SetField(field->SlotIndex, value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API ObjectInstance* Shard_GetArrayElement(ObjectInstance* array, std::size_t index)
    {
        try
        {
            if (array == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return array->GetElement(index);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_SetArrayElement(ObjectInstance* array, std::size_t index, ObjectInstance* value)
    {
        try
        {
            if (array == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            array->SetElement(index, value);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    // =========================================================================
    // GC-less Typed Primitive Field Setters
    // =========================================================================

    SHARD_API int Shard_SetInstanceFieldInteger(ObjectInstance* instance, FieldSymbol* field, std::int64_t value)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            TypeShape* fieldShape = instance->getShape() != nullptr ? instance->getShape()->GetFieldShape(field->SlotIndex) : nullptr;
            ObjectInstance temporary(field->ReturnType, fieldShape, &value, true);
            instance->SetField(field->SlotIndex, &temporary);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetInstanceFieldDouble(ObjectInstance* instance, FieldSymbol* field, double value)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            TypeShape* fieldShape = instance->getShape() != nullptr ? instance->getShape()->GetFieldShape(field->SlotIndex) : nullptr;
            ObjectInstance temporary(field->ReturnType, fieldShape, &value, true);
            instance->SetField(field->SlotIndex, &temporary);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetInstanceFieldBool(ObjectInstance* instance, FieldSymbol* field, int value)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            bool converted = value != 0;
            TypeShape* fieldShape = instance->getShape() != nullptr ? instance->getShape()->GetFieldShape(field->SlotIndex) : nullptr;
            ObjectInstance temporary(field->ReturnType, fieldShape, &converted, true);
            instance->SetField(field->SlotIndex, &temporary);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetInstanceFieldChar(ObjectInstance* instance, FieldSymbol* field, wchar_t value)
    {
        try
        {
            if (instance == nullptr || field == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            TypeShape* fieldShape = instance->getShape() != nullptr ? instance->getShape()->GetFieldShape(field->SlotIndex) : nullptr;
            ObjectInstance temporary(field->ReturnType, fieldShape, &value, true);
            instance->SetField(field->SlotIndex, &temporary);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_GetMethodHandleType(MethodSymbol* method)
    {
        try
        {
            if (method == nullptr)
                return 0;

            return static_cast<int>(method->HandleType);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    SHARD_API int Shard_SetMethodHandleType(MethodSymbol* method, int handleType)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->HandleType = static_cast<MethodHandleType>(handleType);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetInvocationAsExtension(InvokationExpressionSyntax* invocation, int value)
    {
        try
        {
            if (invocation == nullptr)
            {
                SetLastShardWError(L"invocation is null");
                return -1;
            }

            invocation->IsExtensionMethodInvocation = value != 0;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_IsInvocationExtension(InvokationExpressionSyntax* invocation)
    {
        try
        {
            if (invocation == nullptr)
                return 0;

            return invocation->IsExtensionMethodInvocation ? 1 : 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return 0;
        }
    }

    // =========================================================================
    // Native callback binding helpers
    // =========================================================================

    SHARD_API int Shard_SetMethodCallback(MethodSymbol* method, MethodSymbolDelegate callback)
    {
        try
        {
            if (method == nullptr)
            {
                SetLastShardWError(L"method is null");
                return -1;
            }

            method->FunctionPointer = callback;
            method->HandleType = MethodHandleType::External;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetConstructorCallback(ConstructorSymbol* ctor, MethodSymbolDelegate callback)
    {
        try
        {
            if (ctor == nullptr)
            {
                SetLastShardWError(L"constructor is null");
                return -1;
            }

            ctor->FunctionPointer = callback;
            ctor->HandleType = MethodHandleType::External;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_SetAccessorCallback(AccessorSymbol* accessor, MethodSymbolDelegate callback)
    {
        try
        {
            if (accessor == nullptr)
            {
                SetLastShardWError(L"accessor is null");
                return -1;
            }

            accessor->FunctionPointer = callback;
            accessor->HandleType = MethodHandleType::External;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API PropertySymbol* Shard_CreatePropertySymbol(
        CompilationContext* ctx,
        TypeSymbol* parentType,
        const wchar_t* name,
        TypeSymbol* type,
        int isStatic,
        int accessibility)
    {
        try
        {
            if (ctx == nullptr)
            {
                SetLastShardWError(L"compilation context is null");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            SymbolLinking linking = isStatic != 0 ? LINK_STATIC : LINK_INSTANCE;
            PropertySymbol* property = factory.Property(name, type, linking);
            property->Accesibility = accessibility != 0
                ? SymbolAccesibility::Public
                : SymbolAccesibility::Private;
            property->Parent = parentType;

            if (parentType != nullptr)
            {
                property->FullName = parentType->FullName + L"." + name;
                parentType->OnSymbolDeclared(property);
            }
            else
            {
                property->FullName = name;
            }

            return property;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API AccessorSymbol* Shard_PropertyAddGetter(CompilationContext* ctx, PropertySymbol* property)
    {
        try
        {
            if (ctx == nullptr || property == nullptr)
            {
                SetLastShardWError(L"context or property is null");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            AccessorSymbol* getter = factory.Getter(property);
            getter->HandleType = MethodHandleType::External;
            return getter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API AccessorSymbol* Shard_PropertyAddSetter(CompilationContext* ctx, PropertySymbol* property)
    {
        try
        {
            if (ctx == nullptr || property == nullptr)
            {
                SetLastShardWError(L"context or property is null");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            AccessorSymbol* setter = factory.Setter(property);
            setter->HandleType = MethodHandleType::External;
            return setter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API InterfaceSymbol* Shard_CreateInterfaceSymbol(CompilationContext* ctx, NamespaceSymbol* parent, const wchar_t* name, int accessibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            InterfaceSymbol* symbol = factory.Interface(name, static_cast<SymbolAccesibility>(accessibility), parent);
            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);

            if (parent != nullptr)
            {
                parent->OnSymbolDeclared(symbol);
            }
            else
            {
                symbol->FullName = symbol->Name;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_TypeAddInterface(TypeSymbol* type, InterfaceSymbol* interfaceType)
    {
        try
        {
            if (type == nullptr || interfaceType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            type->Interfaces.push_back(interfaceType);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_TypeAddGenericInterface(TypeSymbol* type, GenericTypeSymbol* interfaceType)
    {
        try
        {
            if (type == nullptr || interfaceType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            type->Interfaces.push_back(interfaceType);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API int Shard_ClassSetInterfaceMethodImplementation(ClassSymbol* classType, MethodSymbol* interfaceMethod, MethodSymbol* implementationMethod)
    {
        try
        {
            if (classType == nullptr || interfaceMethod == nullptr || implementationMethod == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            classType->InterfaceMethodMap[interfaceMethod] = implementationMethod;
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    SHARD_API StructSymbol* Shard_CreateStructSymbol(CompilationContext* ctx, NamespaceSymbol* parent, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            StructSymbol* symbol = factory.Struct(name);
            symbol->Accesibility = SymbolAccesibility::Public;

            if (parent != nullptr)
            {
                parent->OnSymbolDeclared(symbol);
            }
            else
            {
                symbol->FullName = symbol->Name;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API EnumSymbol* Shard_CreateEnumSymbol(CompilationContext* ctx, NamespaceSymbol* parent, const wchar_t* name, int isFlags)
    {
        try
        {
            if (ctx == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            EnumSymbol* symbol = factory.Enum(name, isFlags != 0);
            symbol->Accesibility = SymbolAccesibility::Public;

            if (parent != nullptr)
            {
                parent->OnSymbolDeclared(symbol);
            }
            else
            {
                symbol->FullName = symbol->Name;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API FieldSymbol* Shard_AddEnumLiteral(CompilationContext* ctx, EnumSymbol* enumType, const wchar_t* name, std::int64_t value)
    {
        try
        {
            if (ctx == nullptr || enumType == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            FieldSymbol* field = factory.EnumField(name, enumType, value);

            enumType->OnSymbolDeclared(field);
            return field;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API OperatorSymbol* Shard_CreateOperatorSymbol(CompilationContext* ctx, TypeSymbol* parentType, const wchar_t* name, TypeSymbol* returnType, int operatorTokenType, int accessibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || parentType == nullptr || returnType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            OperatorSymbol* symbol = factory.Operator(
                name,
                static_cast<shard::TokenType>(operatorTokenType),
                returnType,
                nullptr,
                {});

            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);
            symbol->Linking = LINK_STATIC;
            symbol->Parent = parentType;

            parentType->OnSymbolDeclared(symbol);
            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API TypeParameterSymbol* Shard_CreateTypeParameterSymbol(CompilationContext* ctx, SyntaxSymbol* parent, const wchar_t* name)
    {
        try
        {
            if (ctx == nullptr || parent == nullptr || name == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            TypeParameterSymbol* symbol = nullptr;

            if (TypeSymbol* typeParent = dynamic_cast<TypeSymbol*>(parent))
            {
                symbol = factory.TypeParameter(name, typeParent);
                typeParent->OnSymbolDeclared(symbol);
            }
            else if (MethodSymbol* methodParent = dynamic_cast<MethodSymbol*>(parent))
            {
                symbol = factory.TypeParameter(name, methodParent);
                methodParent->OnSymbolDeclared(symbol);
            }
            else
            {
                SetLastShardWError(L"parent must be a type or method");
                return nullptr;
            }

            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddTypeParameterConstraint(TypeParameterSymbol* typeParam, TypeSymbol* constraint)
    {
        try
        {
            if (typeParam == nullptr || constraint == nullptr)
                return -1;
            typeParam->Constraints.push_back(constraint);
            return 0;
        }
        catch (...)
        {
            return -1;
        }
    }

    SHARD_API std::size_t Shard_GetTypeParameterConstraintCount(TypeParameterSymbol* typeParam)
    {
        if (typeParam == nullptr)
            return 0;
        return typeParam->Constraints.size();
    }

    SHARD_API TypeSymbol* Shard_GetTypeParameterConstraint(TypeParameterSymbol* typeParam, std::size_t index)
    {
        if (typeParam == nullptr || index >= typeParam->Constraints.size())
            return nullptr;
        return typeParam->Constraints[index];
    }

    SHARD_API ArrayTypeSymbol* Shard_CreateArrayTypeSymbol(CompilationContext* ctx, TypeSymbol* elementType)
    {
        try
        {
            if (ctx == nullptr || elementType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            return factory.Array(elementType);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API GenericTypeSymbol* Shard_CreateGenericTypeSymbol(CompilationContext* ctx, TypeSymbol* underlyingType, TypeSymbol** typeArgs, std::size_t typeArgCount)
    {
        try
        {
            if (ctx == nullptr || underlyingType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());

            if (typeArgs == nullptr || typeArgCount == 0)
                return factory.GenericType(underlyingType);

            std::vector<TypeSymbol*> args(typeArgs, typeArgs + typeArgCount);
            return factory.GenericType(underlyingType, args);
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    shard::TypeSymbol* Shard_GetGenericTypeUnderlyingType(shard::GenericTypeSymbol* generic)
    {
        try
        {
            if (generic == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            return generic->UnderlayingType;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API IndexatorSymbol* Shard_CreateIndexatorSymbol(CompilationContext* ctx, TypeSymbol* parentType, const wchar_t* name, TypeSymbol* returnType, int accessibility)
    {
        try
        {
            if (ctx == nullptr || name == nullptr || parentType == nullptr || returnType == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            IndexatorSymbol* symbol = factory.Indexator(name, returnType);
            symbol->Accesibility = static_cast<SymbolAccesibility>(accessibility);
            symbol->Parent = parentType;
            symbol->ReturnType = returnType;

            parentType->OnSymbolDeclared(symbol);
            return symbol;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API int Shard_AddIndexatorParameter(IndexatorSymbol* indexator, ParameterSymbol* parameter)
    {
        try
        {
            if (indexator == nullptr || parameter == nullptr)
            {
                SetLastShardWError(L"invalid argument");
                return -1;
            }

            indexator->OnSymbolDeclared(parameter);
            return 0;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return -1;
        }
    }

    static void CopyIndexatorParametersToAccessor(IndexatorSymbol* indexator, AccessorSymbol* accessor)
    {
        for (ParameterSymbol* param : indexator->Parameters)
            accessor->Parameters.push_back(param);
    }

    SHARD_API AccessorSymbol* Shard_IndexatorAddGetter(CompilationContext* ctx, IndexatorSymbol* indexator)
    {
        try
        {
            if (ctx == nullptr || indexator == nullptr)
            {
                SetLastShardWError(L"context or indexator is null");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            AccessorSymbol* getter = factory.Accessor(L"get_" + indexator->Name, indexator, true);
            getter->HandleType = MethodHandleType::External;
            CopyIndexatorParametersToAccessor(indexator, getter);

            return getter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    SHARD_API AccessorSymbol* Shard_IndexatorAddSetter(CompilationContext* ctx, IndexatorSymbol* indexator)
    {
        try
        {
            if (ctx == nullptr || indexator == nullptr)
            {
                SetLastShardWError(L"context or indexator is null");
                return nullptr;
            }

            SymbolFactory factory(ctx->GetSemanticModel().Table.get());
            AccessorSymbol* setter = factory.Accessor(L"set_" + indexator->Name, indexator, false);
            setter->HandleType = MethodHandleType::External;

            ParameterSymbol* valueParam = setter->Parameters.empty() ? nullptr : setter->Parameters.back();
            setter->Parameters.clear();

            CopyIndexatorParametersToAccessor(indexator, setter);

            if (valueParam != nullptr)
                setter->Parameters.push_back(valueParam);

            return setter;
        }
        catch (const std::exception& e)
        {
            SetLastErrorFromException(e);
            return nullptr;
        }
    }

    // =========================================================================
    // CallState accessors
    // =========================================================================

    SHARD_API std::size_t Shard_CallStateArgCount(const CallState* state)
    {
        try
        {
            if (state == nullptr)
                return 0;

            return state->Args.size();
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }

    SHARD_API ObjectInstance* Shard_CallStateArg(const CallState* state, std::size_t index)
    {
        try
        {
            if (state == nullptr || index >= state->Args.size())
                return nullptr;

            return state->Args[index];
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    SHARD_API GarbageCollector* Shard_CallStateCollector(const CallState* state)
    {
        try
        {
            if (state == nullptr)
                return nullptr;

            return &state->Collector;
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    SHARD_API MethodSymbol* Shard_CallStateMethod(const CallState* state)
    {
        try
        {
            if (state == nullptr)
                return nullptr;

            return state->Method;
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    SHARD_API CallStackFrame* Shard_CallStateFrame(const CallState* state)
    {
        try
        {
            if (state == nullptr)
                return nullptr;

            return state->Frame;
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    // =========================================================================
    // String length helper
    // =========================================================================

    SHARD_API std::int64_t Shard_ReadStringLength(ObjectInstance* instance)
    {
        try
        {
            if (instance == nullptr)
                return 0;

            return instance->AsStringLength();
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }
}

#endif // !defined(SHARDSCRIPT_STATIC)