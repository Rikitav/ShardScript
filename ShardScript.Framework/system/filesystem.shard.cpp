#include <ShardScript.hpp>
#include <shard/runtime/NativeAsync.hpp>
#include <utilities/Strings.hpp>

#include <fcntl.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>

namespace fs = std::filesystem;
using namespace shard;

// FileInfo
TypeSymbol* shard_FileInfo = nullptr;
FieldSymbol* shard_FileInfo_FullNameBackingField = nullptr;

// DirectoryInfo
TypeSymbol* shard_DirectoryInfo = nullptr;
FieldSymbol* shard_DirectoryInfo_FullNameBackingField = nullptr;

// Async type caches.
TypeSymbol* g_AsyncTask = nullptr;
TypeSymbol* g_AsyncValueTask = nullptr;
TypeParameterSymbol* g_AsyncValueTask_T = nullptr;
ClassSymbol* g_RuntimeException = nullptr;
FieldSymbol* g_RuntimeExceptionMessageField = nullptr;
FieldSymbol* g_RuntimeExceptionStackTraceField = nullptr;

// Stream abstraction symbols (loaded from shard.streams at module load).
TypeSymbol* g_Stream_IStream = nullptr;
TypeSymbol* g_Stream_IReadableStream = nullptr;
TypeSymbol* g_Stream_IWritableStream = nullptr;
TypeSymbol* g_Stream_SeekOrigin = nullptr;

TypeSymbol* g_Stream_CancellationToken = nullptr;
FieldSymbol* g_Stream_CancellationToken_SourceField = nullptr;
FieldSymbol* g_Stream_CancellationTokenSource_CanceledField = nullptr;

// FileStream state.
ClassSymbol* g_FileStream = nullptr;
FieldSymbol* g_FileStream_Path = nullptr;
FieldSymbol* g_FileStream_Handle = nullptr;
FieldSymbol* g_FileStream_IsOpen = nullptr;
FieldSymbol* g_FileStream_CanRead = nullptr;
FieldSymbol* g_FileStream_CanWrite = nullptr;

namespace
{
    static std::wstring pathJoin(std::span<std::wstring> args)
    {
        const std::size_t length = args.size();
        if (length == 0)
            return L"";

        std::size_t total_reserve_size = 0;
        for (std::size_t i = 0; i < length; ++i)
        {
            total_reserve_size += args[i].size() + 1;
        }

        std::wstring final_buffer;
        final_buffer.reserve(total_reserve_size);
        final_buffer += args[0];

        for (std::size_t i = 1; i < length; ++i)
        {
            std::wstring& next_part = args[i];
            fs::path p(std::move(final_buffer));

            p /= next_part;
            final_buffer = p.wstring();
        }

        return final_buffer;
    }

    static std::wstring pathJoin(ArgumentsSpan args)
    {
        const std::size_t length = args.size();
        if (length == 0)
            return L"";

        std::size_t total_reserve_size = 0;
        for (std::size_t i = 0; i < length; ++i)
        {
            if (auto* element = args[i])
                total_reserve_size += wcslen(element->AsString()) + 1;
        }

        std::wstring final_buffer;
        final_buffer.reserve(total_reserve_size);
        final_buffer = args[0]->AsString();

        for (std::size_t i = 1; i < length; ++i)
        {
            std::wstring next_part = args[i]->AsString();
            fs::path p(std::move(final_buffer));

            p /= next_part;
            final_buffer = p.wstring();
        }

        return final_buffer;
    }

    static void EnsureStreamSymbols(SymbolTable* table)
    {
        if (g_Stream_IStream != nullptr)
            return;

        g_Stream_IStream = SemanticModel::FindTypeByName(table, L"io.IStream");
        g_Stream_IReadableStream = SemanticModel::FindTypeByName(table, L"io.IReadableStream");
        g_Stream_IWritableStream = SemanticModel::FindTypeByName(table, L"io.IWritableStream");
        g_Stream_SeekOrigin = SemanticModel::FindTypeByName(table, L"io.SeekOrigin");

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

    static std::fstream* GetFileHandle(ObjectInstance* instance)
    {
        ObjectInstance* handle = instance->GetField(g_FileStream_Handle->SlotIndex);
        if (handle == nullptr || handle == GarbageCollector::NullInstance)
            return nullptr;

        return static_cast<std::fstream*>(handle->AsNint());
    }

    static void CloseFileHandle(ObjectInstance* instance, GarbageCollector& gc)
    {
        ObjectInstance* handle = instance->GetField(g_FileStream_Handle->SlotIndex);
        if (handle != nullptr && handle != GarbageCollector::NullInstance)
        {
            void* ptr = handle->AsNint();
            if (ptr != nullptr)
                delete static_cast<std::fstream*>(ptr);
        }

        instance->SetField(g_FileStream_Handle->SlotIndex, gc.FromNint(nullptr, false));
        instance->SetField(g_FileStream_IsOpen->SlotIndex, gc.FromValue(false));
        instance->SetField(g_FileStream_CanRead->SlotIndex, gc.FromValue(false));
        instance->SetField(g_FileStream_CanWrite->SlotIndex, gc.FromValue(false));
    }

    static std::int64_t FileStreamLength(std::fstream* file)
    {
        auto current = file->tellg();
        file->seekg(0, std::ios::end);

        auto length = file->tellg();
        file->seekg(current);

        return static_cast<std::int64_t>(length);
    }

    static ObjectInstance* OpenFileStream(ObjectInstance* instance, const std::wstring& path, std::int64_t modeValue, std::int64_t accessValue, GarbageCollector& gc)
    {
        bool canRead = false;
        bool canWrite = false;

        switch (accessValue)
        {
            case 1:
            {
                canWrite = true;
                break;
            }

            case 2:
            {
                canRead = true;
                canWrite = true;
                break;
            }

            default:
            {
                canRead = true;
                break;
            }
        }

        std::ios::openmode openMode = std::ios::binary;
        bool mustCreateNew = false;

        switch (modeValue)
        {
            case 0: // CreateNew
            {
                mustCreateNew = true;
                openMode |= std::ios::out | std::ios::trunc;
                break;
            }

            case 1: // Create
            {
                openMode |= std::ios::out | std::ios::trunc;
                break;
            }

            case 2: // Open
            {
                openMode |= (canRead ? std::ios::in : std::ios::openmode(0));
                openMode |= (canWrite ? std::ios::out : std::ios::openmode(0));
                break;
            }

            case 3: // OpenOrCreate
            {
                openMode |= std::ios::out | std::ios::trunc;
                break;
            }

            case 4: // Truncate
            {
                openMode |= std::ios::out | std::ios::trunc;
                break;
            }

            case 5: // Append
            {
                openMode |= std::ios::out | std::ios::app;
                canRead = false;
                break;
            }

            default:
                throw std::runtime_error("Invalid file mode.");
        }

        if (canRead)
            openMode |= std::ios::in;

        if (mustCreateNew && fs::exists(path))
            throw std::runtime_error("File already exists.");

        if (modeValue == 2 && !fs::exists(path))
            throw std::runtime_error("File not found.");

        auto* file = new std::fstream(fs::path(path), openMode);
        if (!file->is_open())
        {
            delete file;
            throw std::runtime_error("Failed to open file.");
        }

        instance->SetField(g_FileStream_Path->SlotIndex, gc.FromValue(path));
        instance->SetField(g_FileStream_Handle->SlotIndex, gc.FromNint(file, false));
        instance->SetField(g_FileStream_IsOpen->SlotIndex, gc.FromValue(true));
        instance->SetField(g_FileStream_CanRead->SlotIndex, gc.FromValue(canRead));
        instance->SetField(g_FileStream_CanWrite->SlotIndex, gc.FromValue(canWrite));

        return instance;
    }

    static void InvokeExternalMethod(MethodSymbol* method, std::initializer_list<ObjectInstance*> args, ApplicationDomain& domain, GarbageCollector& gc)
    {
        std::vector<ObjectInstance*> argVec(args);
        ArgumentsSpan span(argVec);
        CallState context
        {
            domain, domain.GetVirtualMachine().GetProgram(), domain.GetVirtualMachine(), gc, nullptr, method, span
        };

        method->FunctionPointer(context);
    }

    static MethodSymbol* FindTypeMethod(TypeSymbol* type, const wchar_t* name, const std::vector<TypeSymbol*>& parameterTypes)
    {
        std::wstring methodName(name);
        return type->FindMethod(methodName, parameterTypes);
    }

    static void CompleteTask(ObjectInstance* task, ApplicationDomain& domain, GarbageCollector& gc)
    {
        static MethodSymbol* completeMethod = nullptr;
        if (completeMethod == nullptr)
        {
            std::vector<TypeSymbol*> noArgs;
            completeMethod = FindTypeMethod(g_AsyncTask, L"Complete", noArgs);
        }

        if (completeMethod != nullptr)
        {
            InvokeExternalMethod(completeMethod, { task }, domain, gc);
        }
    }

    static void FaultTask(ObjectInstance* task, ObjectInstance* exception, ApplicationDomain& domain, GarbageCollector& gc)
    {
        MethodSymbol* setExceptionMethod = FindTypeMethod(const_cast<TypeSymbol*>(task->getInfo()), L"SetException", { SymbolTable::Primitives::Any });
        if (setExceptionMethod != nullptr)
        {
            InvokeExternalMethod(setExceptionMethod, { task, exception }, domain, gc);
        }
    }

    static void SetValueTaskResult(ObjectInstance* task, ObjectInstance* result, ApplicationDomain& domain, GarbageCollector& gc)
    {
        TypeSymbol* taskType = const_cast<TypeSymbol*>(task->getInfo());

        // The instance's info is the generic class definition (e.g. async.ValueTask);
        // the concrete type argument lives on the shape.  Locate SetResult by name
        // and parameter count rather than by exact type match, because the parameter
        // type is the class's own type parameter T.
        MethodSymbol* setResultMethod = nullptr;
        if (taskType != nullptr)
        {
            for (MethodSymbol* method : taskType->Methods)
            {
                if (method->Name == L"SetResult" && method->Parameters.size() == 1)
                {
                    setResultMethod = method;
                    break;
                }
            }
        }

        if (setResultMethod != nullptr)
        {
            InvokeExternalMethod(setResultMethod, { task, result }, domain, gc);
        }
    }

    static ObjectInstance* CreateRuntimeExceptionMessage(const std::string& message, GarbageCollector& gc)
    {
        ObjectInstance* exception = gc.AllocateInstance(g_RuntimeException);
        exception->SetField(g_RuntimeExceptionMessageField->SlotIndex, gc.FromValue(strings::Utf8ToWide(message)));
        exception->SetField(g_RuntimeExceptionStackTraceField->SlotIndex, GarbageCollector::NullInstance);
        return exception;
    }

    static void FaultTaskWithMessage(ObjectInstance* task, const std::string& message, ApplicationDomain& domain, GarbageCollector& gc)
    {
        ObjectInstance* exception = CreateRuntimeExceptionMessage(message, gc);
        FaultTask(task, exception, domain, gc);
    }
}

// ============================================================================
// class FileInfo
// ============================================================================

static ObjectInstance* shard_fileInfo_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullPath = context.Args[1];

    instance->SetField(shard_FileInfo_FullNameBackingField->SlotIndex, fullPath);
    return instance;
}

static ObjectInstance* shard_fileinfo_Name_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_FileInfo_FullNameBackingField->SlotIndex);

    fs::path path(fullName->AsString());
    return context.Collector.FromValue(path.filename().wstring());
}

static ObjectInstance* shard_fileinfo_Exists_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_FileInfo_FullNameBackingField->SlotIndex);

    bool exists = fs::is_regular_file(fullName->AsString());
    return context.Collector.FromValue(exists);
}

static ObjectInstance* shard_fileinfo_Delete(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_FileInfo_FullNameBackingField->SlotIndex);
    std::wstring pathStr = fullName->AsString();

    if (fs::exists(pathStr))
    {
        if (!fs::is_regular_file(pathStr))
            throw std::runtime_error("Path exists but it is not a regular file.");

        if (!fs::remove(pathStr))
            throw std::runtime_error("Failed to delete file.");
    }

    return nullptr; // void
}

// ============================================================================
// class File
// ============================================================================

static ObjectInstance* shard_file_ReadAllText(const CallState& context) noexcept(false)
{
    std::wstring fileName = context.Args[0]->AsString();
    std::wifstream fileStream = std::wifstream(fs::path(fileName));

    if (!fileStream.is_open())
        throw std::runtime_error("Failed to open text file.");

    std::wstring content((std::istreambuf_iterator<wchar_t>(fileStream)), std::istreambuf_iterator<wchar_t>());
    return context.Collector.FromValue(content);
}

static ObjectInstance* shard_file_WriteAllText(const CallState& context) noexcept(false)
{
    std::wstring fileName = context.Args[0]->AsString();
    std::wstring content = context.Args[1]->AsString();
    std::wofstream fileStream = std::wofstream(fs::path(fileName));

    if (!fileStream.is_open())
        throw std::runtime_error("Failed to open text file.");

    fileStream.write(content.c_str(), content.size());
    if (fileStream.fail())
        throw std::runtime_error("File writing failed.");

    return nullptr; // void
}

static ObjectInstance* shard_file_AppendAllText(const CallState& context) noexcept(false)
{
    std::wstring fileName = context.Args[0]->AsString();
    std::wstring content = context.Args[1]->AsString();
    std::wofstream fileStream(fs::path(fileName), std::ios_base::app);

    if (!fileStream.is_open())
        throw std::runtime_error("Failed to open text file.");

    fileStream << content;
    return nullptr; // void
}

// ----------------------------------------------------------------------------
// ReadAllTextAsync
// ----------------------------------------------------------------------------

struct FsReadTextState
{
    ApplicationDomain* Domain = nullptr;
    ObjectInstance* Task = nullptr;
    uv_fs_t req{};
    uv_file fd = -1;
    std::string content;
    std::vector<char> buffer;
};

static void FsReadTextCleanup(FsReadTextState* state)
{
    state->Domain->GetEventLoop().UnrootTask(state->Task);
    delete state;
}

static void FsReadTextComplete(FsReadTextState* state)
{
    std::wstring wideContent = strings::Utf8ToWide(state->content);
    ObjectInstance* result = state->Domain->GetGarbageCollector().FromValue(wideContent);
    SetValueTaskResult(state->Task, result, *state->Domain, state->Domain->GetGarbageCollector());
    FsReadTextCleanup(state);
}

static void FsReadTextOnClose(uv_fs_t* req)
{
    FsReadTextState* state = static_cast<FsReadTextState*>(req->data);
    uv_fs_req_cleanup(req);

    if (state->fd < 0)
    {
        // Already faulted before close.
        FsReadTextCleanup(state);
        return;
    }

    FsReadTextComplete(state);
}

static void FsReadTextOnRead(uv_fs_t* req)
{
    FsReadTextState* state = static_cast<FsReadTextState*>(req->data);
    ssize_t n = req->result;
    uv_fs_req_cleanup(req);

    if (n < 0)
    {
        FaultTaskWithMessage(state->Task, uv_strerror(static_cast<int>(n)), *state->Domain, state->Domain->GetGarbageCollector());
        state->fd = -1;
        uv_fs_close(req->loop, &state->req, state->fd, FsReadTextOnClose);
        return;
    }

    if (n == 0)
    {
        uv_fs_close(req->loop, &state->req, state->fd, FsReadTextOnClose);
        return;
    }

    state->content.append(state->buffer.data(), static_cast<std::size_t>(n));

    uv_buf_t buf = uv_buf_init(state->buffer.data(), static_cast<unsigned int>(state->buffer.size()));
    uv_fs_read(req->loop, &state->req, state->fd, &buf, 1, -1, FsReadTextOnRead);
}

static void FsReadTextOnOpen(uv_fs_t* req)
{
    FsReadTextState* state = static_cast<FsReadTextState*>(req->data);
    ssize_t result = req->result;
    uv_fs_req_cleanup(req);

    if (result < 0)
    {
        FaultTaskWithMessage(state->Task, uv_strerror(static_cast<int>(result)), *state->Domain, state->Domain->GetGarbageCollector());
        FsReadTextCleanup(state);
        return;
    }

    state->fd = static_cast<uv_file>(result);
    state->buffer.resize(4096);

    uv_buf_t buf = uv_buf_init(state->buffer.data(), static_cast<unsigned int>(state->buffer.size()));
    uv_fs_read(req->loop, &state->req, state->fd, &buf, 1, -1, FsReadTextOnRead);
}

static ObjectInstance* shard_file_ReadAllTextAsync(const CallState& context) noexcept
{
    std::wstring fileName = context.Args[0]->AsString();

    ObjectInstance* task = context.Collector.AllocateGeneric(
        g_AsyncValueTask, { SymbolTable::Primitives::String });

    context.Domain.GetEventLoop().RootTask(task);

    FsReadTextState* state = new FsReadTextState();
    state->Domain = &context.Domain;
    state->Task = task;
    state->req.data = state;

    std::string pathUtf8 = strings::WideToUtf8(fileName);
    uv_fs_open(context.Domain.GetEventLoop().GetLoop(), &state->req, pathUtf8.c_str(), O_RDONLY, 0, FsReadTextOnOpen);

    return task;
}

// ----------------------------------------------------------------------------
// WriteAllTextAsync
// ----------------------------------------------------------------------------

struct FsWriteTextState
{
    ApplicationDomain* Domain = nullptr;
    ObjectInstance* Task = nullptr;
    uv_fs_t req{};
    uv_file fd = -1;
    std::string content;
    bool faulted = false;
};

static void FsWriteTextCleanup(FsWriteTextState* state)
{
    state->Domain->GetEventLoop().UnrootTask(state->Task);
    delete state;
}

static void FsWriteTextOnClose(uv_fs_t* req)
{
    FsWriteTextState* state = static_cast<FsWriteTextState*>(req->data);
    uv_fs_req_cleanup(req);

    if (!state->faulted)
        CompleteTask(state->Task, *state->Domain, state->Domain->GetGarbageCollector());

    FsWriteTextCleanup(state);
}

static void FsWriteTextOnWrite(uv_fs_t* req)
{
    FsWriteTextState* state = static_cast<FsWriteTextState*>(req->data);
    ssize_t n = req->result;
    uv_fs_req_cleanup(req);

    if (n < 0)
    {
        state->faulted = true;
        FaultTaskWithMessage(state->Task, uv_strerror(static_cast<int>(n)), *state->Domain, state->Domain->GetGarbageCollector());
        uv_fs_close(req->loop, &state->req, state->fd, FsWriteTextOnClose);
        return;
    }

    uv_fs_close(req->loop, &state->req, state->fd, FsWriteTextOnClose);
}

static void FsWriteTextOnOpen(uv_fs_t* req)
{
    FsWriteTextState* state = static_cast<FsWriteTextState*>(req->data);
    ssize_t result = req->result;
    uv_fs_req_cleanup(req);

    if (result < 0)
    {
        FaultTaskWithMessage(state->Task, uv_strerror(static_cast<int>(result)), *state->Domain, state->Domain->GetGarbageCollector());
        FsWriteTextCleanup(state);
        return;
    }

    state->fd = static_cast<uv_file>(result);

    uv_buf_t buf = uv_buf_init(state->content.data(), static_cast<unsigned int>(state->content.size()));
    uv_fs_write(req->loop, &state->req, state->fd, &buf, 1, 0, FsWriteTextOnWrite);
}

static ObjectInstance* shard_file_WriteAllTextAsync(const CallState& context) noexcept
{
    std::wstring fileName = context.Args[0]->AsString();
    std::wstring content = context.Args[1]->AsString();

    ObjectInstance* task = context.Collector.AllocateInstance(g_AsyncTask);
    context.Domain.GetEventLoop().RootTask(task);

    FsWriteTextState* state = new FsWriteTextState();
    state->Domain = &context.Domain;
    state->Task = task;
    state->req.data = state;
    state->content = strings::WideToUtf8(content);

    std::string pathUtf8 = strings::WideToUtf8(fileName);
    uv_fs_open(context.Domain.GetEventLoop().GetLoop(), &state->req, pathUtf8.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644, FsWriteTextOnOpen);

    return task;
}

static ObjectInstance* shard_file_Exists(const CallState& context) noexcept(false)
{
    std::wstring fileName = context.Args[0]->AsString();
    bool exists = fs::exists(fileName);
    return context.Collector.FromValue(exists);
}

static ObjectInstance* shard_file_Delete(const CallState& context) noexcept(false)
{
    std::wstring fileName = context.Args[0]->AsString();
    if (fs::exists(fileName) && !fs::remove(fileName))
        throw std::runtime_error("Failed to delete file.");

    return nullptr; // void
}

static ObjectInstance* shard_file_Copy(const CallState& context) noexcept(false)
{
    std::wstring sourceFileName = context.Args[0]->AsString();
    std::wstring destFileName = context.Args[1]->AsString();

    if (!fs::copy_file(sourceFileName.c_str(), destFileName.c_str(), fs::copy_options::overwrite_existing))
        throw std::runtime_error("Failed to copy file.");

    return nullptr; // void
}

static ObjectInstance* shard_file_Move(const CallState& context) noexcept(false)
{
    std::wstring sourceFileName = context.Args[0]->AsString();
    std::wstring destFileName = context.Args[1]->AsString();

    if (!fs::exists(sourceFileName) || !fs::is_regular_file(sourceFileName))
        throw std::runtime_error("Source file does not exist or is not a regular file.");

    if (fs::exists(destFileName) && fs::is_directory(destFileName))
        throw std::runtime_error("Destination path cannot be an existing directory.");

    fs::rename(sourceFileName.c_str(), destFileName.c_str());
    return nullptr; // void
}

// ============================================================================
// class DirectoryInfo
// ============================================================================

static ObjectInstance* shard_directoryinfo_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = context.Args[1];

    instance->SetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex, fullName);
    return instance;
}

static ObjectInstance* shard_directoryinfo_Name_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);

    fs::path p(fullName->AsString());
    return context.Collector.FromValue(p.filename().wstring());
}

static ObjectInstance* shard_directoryinfo_Exists_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);

    bool exists = fs::is_directory(fullName->AsString());
    return context.Collector.FromValue(exists);
}

static ObjectInstance* shard_directoryinfo_Create(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);

    if (fs::exists(fullName->AsString()))
        return nullptr; // void

    if (!fs::create_directories(fullName->AsString()))
        throw std::runtime_error("Failed to create directory.");

    return nullptr; // void
}

static ObjectInstance* shard_directoryinfo_Delete(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* fullName = instance->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);

    if (!fs::remove_all(fullName->AsString()))
        throw std::runtime_error("Failed to delete directory tree.");

    return nullptr; // void
}

static ObjectInstance* shard_string_op_div_directoryinfo_string(const CallState& context) noexcept(false)
{
    ObjectInstance* dirFullName = context.Args[0]->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);
    ObjectInstance* right = context.Args[1];
 
    ObjectInstance* args[] = { dirFullName, right };
    std::wstring final_buffer = pathJoin(args);

    ObjectInstance* resultInstance = context.Collector.AllocateInstance(shard_DirectoryInfo);
    resultInstance->SetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex, context.Collector.FromValue(final_buffer));
    return resultInstance;
}

static ObjectInstance* shard_directoryinfo_op_div_directoryinfo_fileinfo(const CallState& context) noexcept(false)
{
    ObjectInstance* left = context.Args[0];
    ObjectInstance* right = context.Args[1];

    ObjectInstance* dirFullName = left->GetField(shard_DirectoryInfo_FullNameBackingField->SlotIndex);
    ObjectInstance* fileFullName = right->GetField(shard_FileInfo_FullNameBackingField->SlotIndex);

    std::wstring args[] = { std::wstring(dirFullName->AsString()), fs::path(right->AsString()).filename().wstring() };
    std::wstring final_buffer = pathJoin(args);

    ObjectInstance* resultInstance = context.Collector.AllocateInstance(shard_FileInfo);
    resultInstance->SetField(shard_FileInfo_FullNameBackingField->SlotIndex, context.Collector.FromValue(final_buffer));
    return resultInstance;
}

// ============================================================================
// class Directory
// ============================================================================

static ObjectInstance* shard_directory_CreateDirectory(const CallState& context) noexcept(false)
{
    std::wstring path = context.Args[0]->AsString();
    if (!fs::exists(path))
    {
        if (!fs::create_directories(path))
            throw std::runtime_error("Failed to create directory.");
    }

    ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);
    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
    FieldSymbol* field = ownerType->Fields[0];
    instance->SetField(field->SlotIndex, context.Args[0]);
    return instance;
}

static ObjectInstance* shard_directory_Exists(const CallState& context) noexcept(false)
{
    std::wstring path = context.Args[0]->AsString();
    bool exists = fs::exists(path);
    return context.Collector.FromValue(exists);
}

static ObjectInstance* shard_directory_Delete(const CallState& context) noexcept(false)
{
    std::wstring path = context.Args[0]->AsString();
    if (!fs::remove(path))
        throw std::runtime_error("Failed to delete directory.");

    return nullptr; // void
}

// ============================================================================
// class Directory
// ============================================================================

static ObjectInstance* shard_string_op_div_string_string(const CallState& context) noexcept(false)
{
    std::wstring final_buffer = pathJoin(context.Args);
    return context.Collector.FromValue(final_buffer);
}

// ============================================================================
// class Directory
// ============================================================================

static ObjectInstance* shard_path_join(const CallState& context) noexcept(false)
{
    ObjectInstance* pathsArray = context.Args[0];
    const std::size_t length = pathsArray->GetArrayLength();
    if (length == 0)
        return context.Collector.FromValue(L"");

    std::wstring final_buffer = pathJoin(pathsArray->ArrayAsSpan());
    return context.Collector.FromValue(final_buffer);
}

// ============================================================================
// class Path
// ============================================================================

static ObjectInstance* shard_path_GetExtension(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    fs::path p(pathStr);
    return context.Collector.FromValue(p.extension().wstring());
}

static ObjectInstance* shard_path_GetFileName(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    fs::path p(pathStr);
    return context.Collector.FromValue(p.filename().wstring());
}

static ObjectInstance* shard_path_GetFileNameWithoutExtension(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    fs::path p(pathStr);
    return context.Collector.FromValue(p.stem().wstring());
}

static ObjectInstance* shard_path_GetDirectoryName(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    fs::path p(pathStr);
    return context.Collector.FromValue(p.parent_path().wstring());
}

static ObjectInstance* shard_path_HasExtension(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    fs::path p(pathStr);
    return context.Collector.FromValue(p.has_extension());
}

static ObjectInstance* shard_path_ChangeExtension(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    std::wstring newExt = (context.Args[1] == nullptr) ? L"" : context.Args[1]->AsString();
    fs::path p(pathStr);

    if (!newExt.empty() && newExt[0] != L'.')
    {
        newExt = L"." + newExt;
    }

    p.replace_extension(newExt);
    return context.Collector.FromValue(p.wstring());
}

static ObjectInstance* shard_path_GetFullPath(const CallState& context) noexcept(false)
{
    std::wstring pathStr = context.Args[0]->AsString();
    try
    {
        fs::path absolutePath = fs::absolute(fs::path(pathStr));
        return context.Collector.FromValue(absolutePath.wstring());
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to resolve absolute path.");
    }
}

static ObjectInstance* shard_path_DirectorySeparatorChar_get(const CallState& context) noexcept(false)
{
#ifdef _WIN32
    return context.Collector.FromValue(L"\\");
#else
    return context.Collector.FromValue(L"/");
#endif
}

static ObjectInstance* shard_path_AltDirectorySeparatorChar_get(const CallState& context) noexcept(false)
{
#ifdef _WIN32
    return context.Collector.FromValue(L"/");
#else
    return context.Collector.FromValue(L"\\");
#endif
}

static ObjectInstance* shard_path_PathSeparator_get(const CallState& context) noexcept(false)
{
#ifdef _WIN32
    return context.Collector.FromValue(L";");
#else
    return context.Collector.FromValue(L":");
#endif
}

// ============================================================================
// FileStream
// ============================================================================

static ObjectInstance* shard_fileStream_Init2(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring path = context.Args[1]->AsString();
    std::int64_t mode = context.Args[2]->AsInteger();
    return OpenFileStream(instance, path, mode, 2, context.Collector);
}

static ObjectInstance* shard_fileStream_Init3(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring path = context.Args[1]->AsString();
    std::int64_t mode = context.Args[2]->AsInteger();
    std::int64_t access = context.Args[3]->AsInteger();
    return OpenFileStream(instance, path, mode, access, context.Collector);
}

static ObjectInstance* shard_fileStream_Read(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot read from a closed stream.");
    if (!instance->GetField(g_FileStream_CanRead->SlotIndex)->AsBoolean())
        throw std::runtime_error("Stream does not support reading.");
    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    std::vector<char> temp(static_cast<std::size_t>(count));
    file->read(temp.data(), static_cast<std::streamsize>(count));
    std::int64_t read = static_cast<std::int64_t>(file->gcount());

    for (std::int64_t i = 0; i < read; ++i)
    {
        buffer->SetElement(static_cast<std::size_t>(offset + i), context.Collector.FromValue(static_cast<std::uint8_t>(temp[static_cast<std::size_t>(i)])));
    }

    return context.Collector.FromValue(read);
}

static ObjectInstance* shard_fileStream_Write(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* buffer = context.Args[1];
    std::int64_t offset = context.Args[2]->AsInteger();
    std::int64_t count = context.Args[3]->AsInteger();

    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot write to a closed stream.");
    
    if (!instance->GetField(g_FileStream_CanWrite->SlotIndex)->AsBoolean())
        throw std::runtime_error("Stream does not support writing.");
    
    if (offset < 0 || count < 0)
        throw std::runtime_error("Offset and count must be non-negative.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    std::vector<char> temp(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i)
    {
        ObjectInstance* element = buffer->GetElement(static_cast<std::size_t>(offset + i));
        temp[static_cast<std::size_t>(i)] = static_cast<char>(element->AsByte());
    }

    file->write(temp.data(), static_cast<std::streamsize>(count));
    if (file->fail())
        throw std::runtime_error("File write failed.");

    return nullptr;
}

static ObjectInstance* shard_fileStream_Flush(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot flush a closed stream.");

    std::fstream* file = GetFileHandle(instance);
    if (file != nullptr)
        file->flush();

    return nullptr;
}

static ObjectInstance* shard_fileStream_Seek(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t offset = context.Args[1]->AsInteger();
    std::int64_t origin = context.Args[2]->AsInteger();

    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot seek a closed stream.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    std::ios::seekdir dir =
        origin == 1 ? std::ios::cur : 
        origin == 2 ? std::ios::end :
        std::ios::beg;

    file->seekg(offset, dir);
    file->seekp(file->tellg());
    return context.Collector.FromValue(static_cast<std::int64_t>(file->tellg()));
}

static ObjectInstance* shard_fileStream_SetLength(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();

    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot set length on a closed stream.");
    
    if (!instance->GetField(g_FileStream_CanWrite->SlotIndex)->AsBoolean())
        throw std::runtime_error("Stream does not support writing.");

    if (value < 0)
        throw std::runtime_error("Length cannot be negative.");

    std::wstring path = instance->GetField(g_FileStream_Path->SlotIndex)->AsString();
    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    file->close();
    fs::resize_file(path, static_cast<std::uintmax_t>(value));
    file->open(fs::path(path), std::ios::binary | std::ios::in | std::ios::out);

    return nullptr;
}

static ObjectInstance* shard_fileStream_Position_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot access a closed stream.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    return context.Collector.FromValue(static_cast<std::int64_t>(file->tellg()));
}

static ObjectInstance* shard_fileStream_Position_set(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::int64_t value = context.Args[1]->AsInteger();

    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot access a closed stream.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    file->seekg(value, std::ios::beg);
    file->seekp(value, std::ios::beg);
    return nullptr;
}

static ObjectInstance* shard_fileStream_Length_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    if (!instance->GetField(g_FileStream_IsOpen->SlotIndex)->AsBoolean())
        throw std::runtime_error("Cannot access a closed stream.");

    std::fstream* file = GetFileHandle(instance);
    if (file == nullptr)
        throw std::runtime_error("Invalid file handle.");

    return context.Collector.FromValue(FileStreamLength(file));
}

static ObjectInstance* shard_fileStream_Close(const CallState& context) noexcept(false)
{
    CloseFileHandle(context.Args[0], context.Collector);
    return nullptr;
}

static ObjectInstance* shard_fileStream_Dispose(const CallState& context) noexcept(false)
{
    return shard_fileStream_Close(context);
}

// ============================================================================
// FileStream async helpers
// ============================================================================

static ObjectInstance* MakeCanceledTask(const CallState& context)
{
    return shard::FaultedTask(context, L"Operation canceled.");
}

static ObjectInstance* shard_fileStream_ReadAsync_Impl(const CallState& context, ObjectInstance* token)
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
        task->SetField(CLASS_VALUETASK_ExceptionField->SlotIndex, shard::CreateRuntimeException(context.Collector, L"Operation canceled."));
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
        async.RunOnThreadPool([state, collector]()
        {
            if (IsStreamCancellationRequested(state->Token))
            {
                state->Canceled = true;
                return;
            }

            std::fstream* file = GetFileHandle(state->InstanceRef.Instance);
            if (file == nullptr)
            {
                state->Canceled = true;
                return;
            }

            std::vector<char> temp(static_cast<std::size_t>(state->Count));
            file->read(temp.data(), static_cast<std::streamsize>(state->Count));
            state->Result = static_cast<std::int64_t>(file->gcount());

            for (std::int64_t i = 0; i < state->Result; ++i)
            {
                state->BufferRef.Instance->SetElement(static_cast<std::size_t>(state->Offset + i), collector->FromValue(static_cast<std::uint8_t>(temp[static_cast<std::size_t>(i)])));
            }
        }, [stateScope = async.ShareState(), state]() mutable
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

static ObjectInstance* shard_fileStream_WriteAsync_Impl(const CallState& context, ObjectInstance* token)
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

                std::fstream* file = GetFileHandle(state->InstanceRef.Instance);
                if (file == nullptr)
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

                file->write(temp.data(), static_cast<std::streamsize>(state->Count));
                if (file->fail())
                    state->Failed = true;
            }, [stateScope = async.ShareState(), state]() mutable
            {
                shard::AsyncScope async(stateScope);

                if (state->Canceled || IsStreamCancellationRequested(state->Token))
                {
                    async.Fail(L"Operation canceled.");
                    return;
                }

                if (state->Failed)
                {
                    async.Fail(L"File write failed.");
                    return;
                }

                async.Complete();
            });
    });
}

static ObjectInstance* shard_fileStream_FlushAsync_Impl(const CallState& context, ObjectInstance* token)
{
    ObjectInstance* instance = context.Args[0];

    if (IsStreamCancellationRequested(token))
        return MakeCanceledTask(context);

    struct State
    {
        ObjectRef InstanceRef;
        ObjectInstance* Token = nullptr;
        bool Failed = false;
        bool Canceled = false;
    };

    auto state = std::make_shared<State>();
    state->InstanceRef = ObjectRef(instance);
    state->Token = token;

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

                std::fstream* file = GetFileHandle(state->InstanceRef.Instance);
                if (file != nullptr)
                    file->flush();
                else
                    state->Failed = true;
            }, [stateScope = async.ShareState(), state]() mutable
            {
                shard::AsyncScope async(stateScope);

                if (state->Canceled || IsStreamCancellationRequested(state->Token))
                {
                    async.Fail(L"Operation canceled.");
                    return;
                }

                if (state->Failed)
                {
                    async.Fail(L"Invalid file handle.");
                    return;
                }

                async.Complete();
            });
    });
}

static ObjectInstance* shard_fileStream_ReadAsync(const CallState& context) noexcept(false)
{
    return shard_fileStream_ReadAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_fileStream_ReadAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_fileStream_ReadAsync_Impl(context, context.Args[4]);
}

static ObjectInstance* shard_fileStream_WriteAsync(const CallState& context) noexcept(false)
{
    return shard_fileStream_WriteAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_fileStream_WriteAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_fileStream_WriteAsync_Impl(context, context.Args[4]);
}

static ObjectInstance* shard_fileStream_FlushAsync(const CallState& context) noexcept(false)
{
    return shard_fileStream_FlushAsync_Impl(context, nullptr);
}

static ObjectInstance* shard_fileStream_FlushAsync_Cancel(const CallState& context) noexcept(false)
{
    return shard_fileStream_FlushAsync_Impl(context, context.Args[1]);
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.io";
    lib.Description = L"Native implementation of filesystem methods";
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
    SymbolBuilder<NamespaceSymbol> fsNamespace(context, L"filesystem");

    // Core async and runtime exception symbols are now owned by the standard
    // symbol table so they are available before any system library is loaded.
    g_AsyncTask = SymbolTable::StandardTypes::Task;
    g_AsyncValueTask = SymbolTable::StandardTypes::ValueTask;

    // Make sure we have the generic class definition, not the open generic
    // instance (ValueTask<T>) that can appear in the type table.
    if (g_AsyncValueTask != nullptr && g_AsyncValueTask->Kind == SyntaxKind::GenericType)
    {
        GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(g_AsyncValueTask);
        if (generic->UnderlayingType != nullptr)
            g_AsyncValueTask = generic->UnderlayingType;
    }

    if (g_AsyncValueTask != nullptr && !g_AsyncValueTask->TypeParameters.empty())
        g_AsyncValueTask_T = g_AsyncValueTask->TypeParameters[0];

    g_RuntimeException = SymbolTable::StandardTypes::RuntimeException;
    if (g_RuntimeException != nullptr)
    {
        g_RuntimeExceptionMessageField = SymbolTable::StandardTypes::RuntimeExceptionMessageField;
        g_RuntimeExceptionStackTraceField = SymbolTable::StandardTypes::RuntimeExceptionStackTraceField;
    }

    // --- class DirectoryInfo ---
    SymbolBuilder<ClassSymbol> dirInfoClass = fsNamespace.AddClass(L"DirectoryInfo");
    shard_DirectoryInfo = dirInfoClass;

    {
        SymbolBuilder<PropertySymbol> fullNameProp = dirInfoClass
            .AddProperty(L"FullName", TYPE_STRING, LINK_INSTANCE);

        shard_DirectoryInfo_FullNameBackingField = fullNameProp
            .AddBackingField();

        fullNameProp.AddGetter();
    }

    dirInfoClass.AddInit()
        .AddParameter(L"fullPath", TYPE_STRING)
        .SetCallback(&shard_directoryinfo_Init);

    dirInfoClass
        .AddMethod(L"Create", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_directoryinfo_Create);
    
    dirInfoClass
        .AddMethod(L"Delete", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_directoryinfo_Delete);
    
    dirInfoClass
        .AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_directoryinfo_Name_get);
    
    dirInfoClass
        .AddProperty(L"Exists", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_directoryinfo_Exists_get);

    // --- class FileInfo ---
    SymbolBuilder<ClassSymbol> fileInfoClass = fsNamespace.AddClass(L"FileInfo");
    shard_FileInfo = fileInfoClass;

    {
        SymbolBuilder<PropertySymbol> fullNameProp = fileInfoClass
            .AddProperty(L"FullName", TYPE_STRING, LINK_INSTANCE);
    
        shard_FileInfo_FullNameBackingField = fullNameProp
            .AddBackingField();

        fullNameProp.AddGetter();
    }

    fileInfoClass.AddInit()
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_fileInfo_Init);

    fileInfoClass
        .AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_fileinfo_Name_get);

    fileInfoClass
        .AddProperty(L"Exists", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_fileinfo_Exists_get);

    fileInfoClass
        .AddMethod(L"Delete", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_fileinfo_Delete);

    dirInfoClass.AddOperator(TokenType::DivOperator, shard_DirectoryInfo, LINK_STATIC)
        .AddParameter(L"left", shard_DirectoryInfo)
        .AddParameter(L"right", TYPE_STRING)
        .SetCallback(&shard_string_op_div_directoryinfo_string);

    dirInfoClass.AddOperator(TokenType::DivOperator, shard_FileInfo, LINK_STATIC)
        .AddParameter(L"left", shard_DirectoryInfo)
        .AddParameter(L"right", TYPE_STRING)
        .SetCallback(&shard_directoryinfo_op_div_directoryinfo_fileinfo);

    // --- class Directory ---
    SymbolBuilder<ClassSymbol> directoryClass = fsNamespace.AddClass(L"Directory", LINK_STATIC);
    
    directoryClass
        .AddMethod(L"Exists", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_directory_Exists);
    
    directoryClass
        .AddMethod(L"Create", dirInfoClass.Get(), LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_directory_CreateDirectory);
    
    directoryClass
        .AddMethod(L"Delete", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_directory_Delete);

    // --- class File ---
    SymbolBuilder<ClassSymbol> fileClass = fsNamespace.AddClass(L"File", LINK_STATIC);

    fileClass.AddMethod(L"ReadAllText", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_file_ReadAllText);

    fileClass.AddMethod(L"WriteAllText", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .AddParameter(L"content", TYPE_STRING)
        .SetCallback(&shard_file_WriteAllText);

    GenericTypeSymbol* valueTaskOfString = nullptr;
    {
        SymbolFactory factory(context.GetSemanticModel().Table.get());
        valueTaskOfString = factory.GenericType(g_AsyncValueTask, { { L"T", SymbolTable::Primitives::String } });
    }

    fileClass.AddMethod(L"ReadAllTextAsync", valueTaskOfString, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_file_ReadAllTextAsync);

    fileClass.AddMethod(L"WriteAllTextAsync", g_AsyncTask, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .AddParameter(L"content", TYPE_STRING)
        .SetCallback(&shard_file_WriteAllTextAsync);

    fileClass.AddMethod(L"AppendAllText", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .AddParameter(L"content", TYPE_STRING)
        .SetCallback(&shard_file_AppendAllText);

    fileClass.AddMethod(L"Exists", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_file_Exists);

    fileClass.AddMethod(L"Delete", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_file_Delete);

    fileClass.AddMethod(L"Copy", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"sourceFileName", TYPE_STRING)
        .AddParameter(L"destFileName", TYPE_STRING)
        .SetCallback(&shard_file_Copy);

    fileClass.AddMethod(L"Move", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"sourceFileName", TYPE_STRING)
        .AddParameter(L"destFileName", TYPE_STRING)
        .SetCallback(&shard_file_Move);

    // --- stream symbols ---
    EnsureStreamSymbols(context.GetSemanticModel().Table.get());

    GenericTypeSymbol* valueTaskOfInt = nullptr;
    {
        SymbolFactory factory(context.GetSemanticModel().Table.get());
        valueTaskOfInt = factory.GenericType(g_AsyncValueTask, { { L"T", TYPE_INT } });
    }

    // --- enum FileMode ---
    {
        SymbolBuilder<EnumSymbol> fileModeEnum = fsNamespace.AddEnum(L"FileMode", false, ACS_PUBLIC);
        fileModeEnum
            .AddValue(L"CreateNew", 0)
            .AddValue(L"Create", 1)
            .AddValue(L"Open", 2)
            .AddValue(L"OpenOrCreate", 3)
            .AddValue(L"Truncate", 4)
            .AddValue(L"Append", 5);
    }

    // --- enum FileAccess ---
    {
        SymbolBuilder<EnumSymbol> fileAccessEnum = fsNamespace.AddEnum(L"FileAccess", false, ACS_PUBLIC);
        fileAccessEnum
            .AddValue(L"Read", 0)
            .AddValue(L"Write", 1)
            .AddValue(L"ReadWrite", 2);
    }

    // --- class FileStream ---
    {
        SymbolFactory factory(context.GetSemanticModel().Table.get());
        TypeSymbol* fileModeType = SemanticModel::FindTypeByName(context.GetSemanticModel().Table.get(), L"filesystem.FileMode");
        TypeSymbol* fileAccessType = SemanticModel::FindTypeByName(context.GetSemanticModel().Table.get(), L"filesystem.FileAccess");

        SymbolBuilder<ClassSymbol> fileStreamClass = fsNamespace.AddClass(L"FileStream");
        g_FileStream = fileStreamClass;
        fileStreamClass.Implements(static_cast<InterfaceSymbol*>(g_Stream_IReadableStream));
        fileStreamClass.Implements(static_cast<InterfaceSymbol*>(g_Stream_IWritableStream));
        fileStreamClass.Implements(TRAIT_DISPOSABLE);

        g_FileStream_Path = fileStreamClass
            .AddField(L"_path", TYPE_STRING, LINK_INSTANCE, ACS_PRIVATE);

        g_FileStream_Handle = fileStreamClass
            .AddField(L"_handle", TYPE_NINT, LINK_INSTANCE, ACS_PRIVATE);

        g_FileStream_IsOpen = fileStreamClass
            .AddField(L"_isOpen", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        g_FileStream_CanRead = fileStreamClass
            .AddField(L"_canRead", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        g_FileStream_CanWrite = fileStreamClass
            .AddField(L"_canWrite", TYPE_BOOL, LINK_INSTANCE, ACS_PRIVATE);

        fileStreamClass.AddInit()
            .AddParameter(L"path", TYPE_STRING)
            .AddParameter(L"mode", fileModeType != nullptr ? fileModeType : TYPE_INT)
            .SetCallback(&shard_fileStream_Init2);

        fileStreamClass.AddInit()
            .AddParameter(L"path", TYPE_STRING)
            .AddParameter(L"mode", fileModeType != nullptr ? fileModeType : TYPE_INT)
            .AddParameter(L"access", fileAccessType != nullptr ? fileAccessType : TYPE_INT)
            .SetCallback(&shard_fileStream_Init3);

        fileStreamClass.AddMethod(L"Read", TYPE_INT, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_fileStream_Read);

        fileStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_fileStream_ReadAsync);

        fileStreamClass.AddMethod(L"ReadAsync", valueTaskOfInt, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_fileStream_ReadAsync_Cancel);

        fileStreamClass.AddMethod(L"Write", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_fileStream_Write);

        fileStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .SetCallback(&shard_fileStream_WriteAsync);

        fileStreamClass.AddMethod(L"WriteAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"buffer", factory.Array(TYPE_BYTE))
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"count", TYPE_INT)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_fileStream_WriteAsync_Cancel);

        fileStreamClass.AddMethod(L"Flush", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_fileStream_Flush);

        fileStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .SetCallback(&shard_fileStream_FlushAsync);

        fileStreamClass.AddMethod(L"FlushAsync", CLASS_TASK, LINK_INSTANCE)
            .AddParameter(L"cancellationToken", g_Stream_CancellationToken)
            .SetCallback(&shard_fileStream_FlushAsync_Cancel);

        fileStreamClass.AddMethod(L"Seek", TYPE_INT, LINK_INSTANCE)
            .AddParameter(L"offset", TYPE_INT)
            .AddParameter(L"origin", g_Stream_SeekOrigin != nullptr ? g_Stream_SeekOrigin : TYPE_INT)
            .SetCallback(&shard_fileStream_Seek);

        fileStreamClass.AddMethod(L"SetLength", TYPE_VOID, LINK_INSTANCE)
            .AddParameter(L"value", TYPE_INT)
            .SetCallback(&shard_fileStream_SetLength);

        fileStreamClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_fileStream_Close);

        fileStreamClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
            .SetCallback(&shard_fileStream_Dispose);

        SymbolBuilder<PropertySymbol> positionProp = fileStreamClass.AddProperty(L"Position", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
        positionProp.AddGetter()
            .SetCallback(&shard_fileStream_Position_get);
        positionProp.AddSetter()
            .SetCallback(&shard_fileStream_Position_set);

        SymbolBuilder<PropertySymbol> lengthProp = fileStreamClass.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC);
        lengthProp.AddGetter()
            .SetCallback(&shard_fileStream_Length_get);

        fileStreamClass.DeclareGlobal();
    }

    // --- class Path ---
    SymbolBuilder<ClassSymbol> pathClass = fsNamespace.AddClass(L"Path", LINK_STATIC);

    pathClass.AddMethod(L"Join", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"paths", pathClass.GetFactory().Array(TYPE_STRING))
        .SetCallback(&shard_path_join);

    pathClass.AddMethod(L"GetExtension", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_GetExtension);

    pathClass.AddMethod(L"GetFileName", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_GetFileName);

    pathClass.AddMethod(L"GetFileNameWithoutExtension", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_GetFileNameWithoutExtension);

    pathClass.AddMethod(L"GetDirectoryName", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_GetDirectoryName);

    pathClass.AddMethod(L"HasExtension", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_HasExtension);

    pathClass.AddMethod(L"ChangeExtension", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .AddParameter(L"extension", TYPE_STRING)
        .SetCallback(&shard_path_ChangeExtension);

    pathClass.AddMethod(L"GetFullPath", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"path", TYPE_STRING)
        .SetCallback(&shard_path_GetFullPath);

    pathClass.AddProperty(L"DirectorySeparatorChar", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_path_DirectorySeparatorChar_get);

    pathClass.AddProperty(L"AltDirectorySeparatorChar", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_path_AltDirectorySeparatorChar_get);

    pathClass.AddProperty(L"PathSeparator", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
        .AddGetter()
        .SetCallback(&shard_path_PathSeparator_get);

    // --- string extensions ---
    SymbolBuilder<ClassSymbol> stringClass = SymbolBuilder<ClassSymbol>(context, static_cast<ClassSymbol*>(TYPE_STRING));

    stringClass.AddOperator(TokenType::DivOperator, TYPE_STRING, LINK_STATIC)
        .AddParameter(L"left", TYPE_STRING)
        .AddParameter(L"right", TYPE_STRING)
        .SetCallback(&shard_string_op_div_string_string);
}