#include <ShardScript.hpp>
#include "subprocess.h"

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cwctype>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace shard;

// ============================================================================
// Symbol handles
// ============================================================================

static ClassSymbol* shard_Process = nullptr;
static FieldSymbol* shard_Process_handle = nullptr;
static FieldSymbol* shard_Process_exitCode = nullptr;
static FieldSymbol* shard_Process_redirectOut = nullptr;
static FieldSymbol* shard_Process_redirectErr = nullptr;
static FieldSymbol* shard_Process_redirectIn = nullptr;
static FieldSymbol* shard_Process_processId = nullptr;

static ClassSymbol* shard_ProcessStartInfo = nullptr;
static FieldSymbol* shard_StartInfo_fileName = nullptr;
static FieldSymbol* shard_StartInfo_arguments = nullptr;
static FieldSymbol* shard_StartInfo_workingDir = nullptr;
static FieldSymbol* shard_StartInfo_redirectOut = nullptr;
static FieldSymbol* shard_StartInfo_redirectErr = nullptr;
static FieldSymbol* shard_StartInfo_redirectIn = nullptr;
static FieldSymbol* shard_StartInfo_useShell = nullptr;
static FieldSymbol* shard_StartInfo_createNoWindow = nullptr;
static FieldSymbol* shard_StartInfo_inheritEnv = nullptr;
static FieldSymbol* shard_StartInfo_environment = nullptr;

// Dictionary<string,string> layout used for environment variables.
static ClassSymbol* dictionaryClass_raw = nullptr;
static FieldSymbol* dict_keysField = nullptr;
static FieldSymbol* dict_valuesField = nullptr;
static FieldSymbol* dict_statesField = nullptr;
static FieldSymbol* dict_countField = nullptr;

// ============================================================================
// Small helpers for reading ShardScript fields safely
// ============================================================================

static ClassSymbol* FindDictionaryClass(CompilationContext& context)
{
    for (TypeSymbol* type : context.GetSemanticModel().Table->GetTypeSymbols())
    {
        if (type->Name == L"Dictionary" && type->FullName == L"collections.Dictionary")
            return static_cast<ClassSymbol*>(type);
    }
    return nullptr;
}

static bool IsNullInstance(ObjectInstance* instance)
{
    return instance == nullptr || instance == GarbageCollector::NullInstance;
}

static bool GetBoolField(ObjectInstance* instance, FieldSymbol* field)
{
    ObjectInstance* value = instance->GetField(field->SlotIndex);
    if (IsNullInstance(value))
        return false;

    return value->AsBoolean();
}

static const wchar_t* GetStringField(ObjectInstance* instance, FieldSymbol* field)
{
    ObjectInstance* value = instance->GetField(field->SlotIndex);
    if (IsNullInstance(value))
        return L"";

    return value->AsString();
}

static subprocess_s* GetProcessHandle(ObjectInstance* instance)
{
    ObjectInstance* handleField = instance->GetField(shard_Process_handle->SlotIndex);
    if (IsNullInstance(handleField))
        return nullptr;

    return static_cast<subprocess_s*>(handleField->AsNint());
}

static void SetProcessHandle(ObjectInstance* instance, subprocess_s* proc, GarbageCollector& gc)
{
    instance->SetField(shard_Process_handle->SlotIndex, gc.FromNint(proc, false));
}

static void SetExitCode(ObjectInstance* instance, std::int64_t code, GarbageCollector& gc)
{
    instance->SetField(shard_Process_exitCode->SlotIndex, gc.FromValue(code));
}

static void SetRedirectFlags(ObjectInstance* instance, bool redirectOut, bool redirectErr, bool redirectIn, GarbageCollector& gc)
{
    instance->SetField(shard_Process_redirectOut->SlotIndex, gc.FromValue(redirectOut));
    instance->SetField(shard_Process_redirectErr->SlotIndex, gc.FromValue(redirectErr));
    instance->SetField(shard_Process_redirectIn->SlotIndex, gc.FromValue(redirectIn));
}

static bool GetProcessRedirectFlag(ObjectInstance* instance, FieldSymbol* field)
{
    ObjectInstance* value = instance->GetField(field->SlotIndex);
    if (IsNullInstance(value))
        return false;

    return value->AsBoolean();
}

static void SetProcessId(ObjectInstance* instance, std::int64_t pid, GarbageCollector& gc)
{
    instance->SetField(shard_Process_processId->SlotIndex, gc.FromValue(pid));
}

// ============================================================================
// Core process creation
// ============================================================================

static std::vector<std::wstring> SplitArguments(const std::wstring& arguments)
{
    std::vector<std::wstring> result;
    std::wstring current;
    bool inDouble = false;
    bool inSingle = false;

    for (wchar_t ch : arguments)
    {
        if (ch == L'"' && !inSingle)
        {
            inDouble = !inDouble;
            continue;
        }

        if (ch == L'\'' && !inDouble)
        {
            inSingle = !inSingle;
            continue;
        }

        if (std::iswspace(ch) && !inDouble && !inSingle)
        {
            if (!current.empty())
            {
                result.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(ch);
        }
    }

    if (!current.empty())
        result.push_back(current);

    return result;
}

static std::vector<std::string> BuildCommandLine(const wchar_t* fileName, const wchar_t* arguments)
{
    std::vector<std::string> argv;
    argv.push_back(strings::WideToUtf8(fileName));

    if (arguments != nullptr && arguments[0] != L'\0')
    {
        for (const std::wstring& arg : SplitArguments(arguments))
            argv.push_back(strings::WideToUtf8(arg));
    }

    return argv;
}

static ObjectInstance* StartProcess(
    GarbageCollector& gc,
    const std::vector<std::string>& argvUtf8,
    bool inheritEnvironment,
    const std::string& cwdUtf8,
    bool createNoWindow,
    bool redirectStandardOutput,
    bool redirectStandardError,
    bool redirectStandardInput,
    const std::vector<std::string>& environmentUtf8)
{
    if (argvUtf8.empty())
        throw std::runtime_error("Process.Start requires a file name.");

    std::vector<const char*> commandLine;
    commandLine.reserve(argvUtf8.size() + 1);
    
    for (const std::string& arg : argvUtf8)
        commandLine.push_back(arg.c_str());
    
    commandLine.push_back(nullptr);

    int options = subprocess_option_search_user_path;
    if (inheritEnvironment)
        options |= subprocess_option_inherit_environment;
    
    if (createNoWindow)
        options |= subprocess_option_no_window;

    // Always enable async reads when any redirect is requested; also keep async
    // enabled for the existing blocking ReadToEnd semantics to work reliably.
    if (redirectStandardOutput || redirectStandardError || redirectStandardInput)
        options |= subprocess_option_enable_async;
    else
        options |= subprocess_option_enable_async;

    const char* const* environmentPtr = nullptr;
    std::vector<const char*> environmentVector;
    if (!inheritEnvironment && !environmentUtf8.empty())
    {
        environmentVector.reserve(environmentUtf8.size() + 1);
        for (const std::string& env : environmentUtf8)
            environmentVector.push_back(env.c_str());
        environmentVector.push_back(nullptr);
        environmentPtr = environmentVector.data();
    }

    subprocess_s* proc = new subprocess_s();
    const int result = subprocess_create_ex(
        commandLine.data(),
        options,
        environmentPtr,
        cwdUtf8.empty() ? nullptr : cwdUtf8.c_str(),
        proc);

    if (result != subprocess_error_success)
    {
        delete proc;
        throw std::runtime_error("Failed to start process (error code: " + std::to_string(result) + ")");
    }

    ObjectInstance* process = gc.AllocateInstance(shard_Process);
    SetProcessHandle(process, proc, gc);
    SetExitCode(process, 0, gc);
    SetRedirectFlags(process, redirectStandardOutput, redirectStandardError, redirectStandardInput, gc);

#ifdef _WIN32
    SetProcessId(process, static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(proc->hProcess)), gc);
#else
    SetProcessId(process, static_cast<std::int64_t>(proc->child), gc);
#endif

    return process;
}

// ============================================================================
// ProcessStartInfo
// ============================================================================

static ObjectInstance* shard_ProcessStartInfo_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    instance->SetField(shard_StartInfo_inheritEnv->SlotIndex, context.Collector.FromValue(true));

    ObjectInstance* envDict = context.Collector.AllocateGeneric(dictionaryClass_raw,
        { SymbolTable::Primitives::String, SymbolTable::Primitives::String });
    instance->SetField(shard_StartInfo_environment->SlotIndex, envDict);

    return instance;
}

// ============================================================================
// Process static Start methods
// ============================================================================

static ObjectInstance* shard_Process_Start_FileName(const CallState& context) noexcept(false)
{
    const wchar_t* fileName = context.Args[0]->AsString();
    std::vector<std::string> argv = BuildCommandLine(fileName, L"");
    return StartProcess(context.Collector, argv, true, {}, false, false, false, false, {});
}

static ObjectInstance* shard_Process_Start_FileNameArgs(const CallState& context) noexcept(false)
{
    const wchar_t* fileName = context.Args[0]->AsString();
    const wchar_t* arguments = context.Args[1]->AsString();
    std::vector<std::string> argv = BuildCommandLine(fileName, arguments);
    return StartProcess(context.Collector, argv, true, {}, false, false, false, false, {});
}

static ObjectInstance* shard_Process_Start_Info(const CallState& context) noexcept(false)
{
    ObjectInstance* info = context.Args[0];

    const wchar_t* fileName = GetStringField(info, shard_StartInfo_fileName);
    if (fileName[0] == L'\0')
        throw std::runtime_error("ProcessStartInfo.FileName is required.");

    if (GetBoolField(info, shard_StartInfo_useShell))
        throw std::runtime_error("UseShellExecute is not supported by the subprocess library.");

    std::vector<std::string> argv = BuildCommandLine(
        fileName,
        GetStringField(info, shard_StartInfo_arguments));

    std::string cwdUtf8 = strings::WideToUtf8(GetStringField(info, shard_StartInfo_workingDir));

    bool inheritEnv = GetBoolField(info, shard_StartInfo_inheritEnv);
    bool createNoWindow = GetBoolField(info, shard_StartInfo_createNoWindow);
    bool redirectOut = GetBoolField(info, shard_StartInfo_redirectOut);
    bool redirectErr = GetBoolField(info, shard_StartInfo_redirectErr);
    bool redirectIn = GetBoolField(info, shard_StartInfo_redirectIn);

    std::vector<std::string> environmentVars;
    ObjectInstance* envDict = info->GetField(shard_StartInfo_environment->SlotIndex);
    if (!IsNullInstance(envDict) && envDict->GetField(dict_countField->SlotIndex)->AsInteger() > 0)
    {
        ObjectInstance* keys = envDict->GetField(dict_keysField->SlotIndex);
        ObjectInstance* values = envDict->GetField(dict_valuesField->SlotIndex);
        ObjectInstance* states = envDict->GetField(dict_statesField->SlotIndex);
        std::size_t capacity = states ? states->GetArrayLength() : 0;

        for (std::size_t i = 0; i < capacity; ++i)
        {
            if (states->GetElement(i, context.Frame)->AsInteger() != 1)
                continue;

            std::wstring key = keys->GetElement(i, context.Frame)->AsString();
            std::wstring value = values->GetElement(i, context.Frame)->AsString();
            environmentVars.push_back(strings::WideToUtf8(key + L"=" + value));
        }
    }

    return StartProcess(context.Collector, argv, inheritEnv, cwdUtf8, createNoWindow, redirectOut, redirectErr, redirectIn, environmentVars);
}

// ============================================================================
// Process instance members
// ============================================================================

static ObjectInstance* shard_Process_HasExited_get(const CallState& context) noexcept(false)
{
    subprocess_s* proc = GetProcessHandle(context.Args[0]);
    if (proc == nullptr)
        return context.Collector.FromValue(true);

    return context.Collector.FromValue(subprocess_alive(proc) == 0);
}

static ObjectInstance* shard_Process_ExitCode_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    return instance->GetField(shard_Process_exitCode->SlotIndex);
}

static ObjectInstance* shard_Process_ProcessId_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    return instance->GetField(shard_Process_processId->SlotIndex);
}

static ObjectInstance* shard_Process_WaitForExit(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    int code = 0;
    if (subprocess_join(proc, &code) != 0)
        throw std::runtime_error("Failed to wait for process.");

    SetExitCode(instance, static_cast<std::int64_t>(code), context.Collector);
    return context.Collector.FromValue(static_cast<std::int64_t>(code));
}

static ObjectInstance* shard_Process_WaitForExitTimeout(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    std::int64_t timeoutMs = context.Args[1]->AsInteger();
    if (timeoutMs < 0)
        return shard_Process_WaitForExit(context);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (subprocess_alive(proc) != 0)
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return context.Collector.FromValue(false);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int code = 0;
    subprocess_join(proc, &code);
    SetExitCode(instance, static_cast<std::int64_t>(code), context.Collector);
    return context.Collector.FromValue(true);
}

static ObjectInstance* shard_Process_Kill(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    if (subprocess_terminate(proc) != 0)
        throw std::runtime_error("Failed to terminate process.");

    SetExitCode(instance, -1, context.Collector);
    return nullptr;
}

static ObjectInstance* shard_Process_ReadToEnd(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    FILE* out = subprocess_stdout(proc);
    if (out == nullptr)
        throw std::runtime_error("Standard output is not available.");

    std::string output;
    char buffer[4096];
    unsigned bytesRead = 0;

    do
    {
        bytesRead = subprocess_read_stdout(proc, buffer, sizeof(buffer));
        if (bytesRead > 0)
            output.append(buffer, bytesRead);
    }
    while (bytesRead > 0);

    int code = 0;
    subprocess_join(proc, &code);
    SetExitCode(instance, static_cast<std::int64_t>(code), context.Collector);

    return context.Collector.FromValue(strings::Utf8ToWide(output));
}

static ObjectInstance* shard_Process_ReadErrorToEnd(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    FILE* err = subprocess_stderr(proc);
    if (err == nullptr)
        throw std::runtime_error("Standard error is not available.");

    std::string output;
    char buffer[4096];
    unsigned bytesRead = 0;

    do
    {
        bytesRead = subprocess_read_stderr(proc, buffer, sizeof(buffer));
        if (bytesRead > 0)
            output.append(buffer, bytesRead);
    }
    while (bytesRead > 0);

    int code = 0;
    subprocess_join(proc, &code);
    SetExitCode(instance, static_cast<std::int64_t>(code), context.Collector);

    return context.Collector.FromValue(strings::Utf8ToWide(output));
}

static ObjectInstance* shard_Process_Write(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    const wchar_t* text = context.Args[1]->AsString();
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc == nullptr)
        throw std::runtime_error("Process handle is invalid.");

    FILE* in = subprocess_stdin(proc);
    if (in == nullptr)
        throw std::runtime_error("Standard input is not available.");

    std::string narrow = strings::WideToUtf8(text);
    if (!narrow.empty())
    {
        std::fwrite(narrow.data(), 1, narrow.size(), in);
        std::fflush(in);
    }

    return nullptr;
}

static ObjectInstance* shard_Process_WriteLine(const CallState& context) noexcept(false)
{
    shard_Process_Write(context);

    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc != nullptr)
    {
        FILE* in = subprocess_stdin(proc);
        if (in != nullptr)
        {
            const char newline[] = "\n";
            std::fwrite(newline, 1, 1, in);
            std::fflush(in);
        }
    }

    return nullptr;
}

static ObjectInstance* shard_Process_Dispose(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    subprocess_s* proc = GetProcessHandle(instance);
    if (proc != nullptr)
    {
        subprocess_destroy(proc);
        delete proc;
        SetProcessHandle(instance, nullptr, context.Collector);
    }

    return nullptr;
}

// ============================================================================
// Metadata & entry point
// ============================================================================

SHARDLIB_GETMETADATA
{
    static const shard::ShardLibDependencyInfo deps[] =
    {
        { L"shard.collections", L"^0.3.0" }
    };

    lib.Name = L"shard.subprocess";
    lib.Description = L"ShardScript process spawning library";
    lib.Version = L"1.0.0";
    lib.Dependencies = deps;
    lib.DependenciesLength = sizeof(deps) / sizeof(deps[0]);
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> processNamespace(context, L"process");

    // Locate the collections.Dictionary<string,string> layout we need for
    // environment variables. This avoids a cross-DLL static-symbol dependency.
    dictionaryClass_raw = FindDictionaryClass(context);
    if (dictionaryClass_raw != nullptr)
    {
        dict_keysField   = SemanticModel::FindFieldByName(dictionaryClass_raw, L"_keys");
        dict_valuesField = SemanticModel::FindFieldByName(dictionaryClass_raw, L"_values");
        dict_statesField = SemanticModel::FindFieldByName(dictionaryClass_raw, L"_states");
        dict_countField  = SemanticModel::FindFieldByName(dictionaryClass_raw, L"_count");
    }

    // ------------------------------------------------------------------------
    // class ProcessStartInfo
    // ------------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> startInfoClass = processNamespace.AddClass(L"ProcessStartInfo");
    shard_ProcessStartInfo = startInfoClass;

    shard_StartInfo_fileName = startInfoClass
        .AddField(L"FileName", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_arguments = startInfoClass
        .AddField(L"Arguments", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_workingDir = startInfoClass
        .AddField(L"WorkingDirectory", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_redirectOut = startInfoClass
        .AddField(L"RedirectStandardOutput", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_redirectErr = startInfoClass
        .AddField(L"RedirectStandardError", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_redirectIn = startInfoClass
        .AddField(L"RedirectStandardInput", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_useShell = startInfoClass
        .AddField(L"UseShellExecute", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_createNoWindow = startInfoClass
        .AddField(L"CreateNoWindow", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_inheritEnv = startInfoClass
        .AddField(L"InheritEnvironment", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC);

    shard_StartInfo_environment = startInfoClass
        .AddField(L"EnvironmentVariables", 
            startInfoClass.GetFactory().GenericType(dictionaryClass_raw, { { L"K", SymbolTable::Primitives::String }, { L"V", SymbolTable::Primitives::String } }),
            LINK_INSTANCE, ACS_PUBLIC);

    startInfoClass.AddInit()
        .SetCallback(&shard_ProcessStartInfo_Init);

    // ------------------------------------------------------------------------
    // class Process
    // ------------------------------------------------------------------------
    SymbolBuilder<ClassSymbol> processClass = processNamespace.AddClass(L"Process");
    shard_Process = processClass.Implements(TRAIT_DISPOSABLE);

    shard_Process_handle = processClass
        .AddField(L"_handle", TYPE_NINT, LINK_INSTANCE, ACS_PRIVATE);
    
    shard_Process_exitCode = processClass
        .AddField(L"_exitCode", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);
    
    shard_Process_redirectOut = processClass
        .AddField(L"_redirectOut", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);
    
    shard_Process_redirectErr = processClass
        .AddField(L"_redirectErr", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);
    
    shard_Process_redirectIn = processClass
        .AddField(L"_redirectIn", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

    shard_Process_processId = processClass
        .AddField(L"_processId", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    processClass.AddMethod(L"Start", shard_Process, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_Process_Start_FileName);

    processClass.AddMethod(L"Start", shard_Process, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .AddParameter(L"arguments", TYPE_STRING)
        .SetCallback(&shard_Process_Start_FileNameArgs);

    processClass.AddMethod(L"Start", shard_Process, LINK_STATIC)
        .AddParameter(L"startInfo", shard_ProcessStartInfo)
        .SetCallback(&shard_Process_Start_Info);

    processClass.AddProperty(L"HasExited", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_Process_HasExited_get);

    processClass.AddProperty(L"ExitCode", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_Process_ExitCode_get);

    processClass.AddProperty(L"ProcessId", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_Process_ProcessId_get);

    processClass.AddMethod(L"WaitForExit", TYPE_INT, LINK_INSTANCE)
        .SetCallback(&shard_Process_WaitForExit);

    processClass.AddMethod(L"WaitForExit", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"timeoutMilliseconds", TYPE_INT)
        .SetCallback(&shard_Process_WaitForExitTimeout);

    processClass.AddMethod(L"Kill", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_Process_Kill);

    processClass.AddMethod(L"ReadToEnd", TYPE_STRING, LINK_INSTANCE)
        .SetCallback(&shard_Process_ReadToEnd);

    processClass.AddMethod(L"ReadErrorToEnd", TYPE_STRING, LINK_INSTANCE)
        .SetCallback(&shard_Process_ReadErrorToEnd);

    processClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"text", TYPE_STRING)
        .SetCallback(&shard_Process_Write);

    processClass.AddMethod(L"WriteLine", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"text", TYPE_STRING)
        .SetCallback(&shard_Process_WriteLine);

    processClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_Process_Dispose)
        .IsImplementationOf(TRAIT_DISPOSABLE_Dispose);
}
