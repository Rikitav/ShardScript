#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace shard;

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

    /*
    if (!fs::copy_file(sourceFileName.c_str(), destFileName.c_str(), fs::copy_options::overwrite_existing))
        throw std::runtime_error("Failed to move file.");

    if (!fs::remove(sourceFileName.c_str()))
        throw std::runtime_error("Failed to delete original file after move.");
    */

    if (!fs::is_regular_file(sourceFileName))
        throw std::runtime_error("Source is not a file");

    if (!fs::is_regular_file(destFileName))
        throw std::runtime_error("Destination is not a file");

    fs::rename(sourceFileName.c_str(), destFileName.c_str());
    return nullptr; // void
}

// ============================================================================
// class DirectoryInfo
// ============================================================================

static ObjectInstance* shard_directory_GetDirectory(const CallState& context) noexcept(false)
{
    ObjectInstance* fullName = context.Args[0];
    ObjectInstance* instance = context.Collector.AllocateInstance(context.Method->ReturnType);

    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());
    FieldSymbol* field = ownerType->Fields[0];

    instance->SetField(field, fullName);
    return instance;
}

static ObjectInstance* shard_directory_Name_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

    FieldSymbol* field = ownerType->Fields[0];
    ObjectInstance* fullName = instance->GetField(field);
    std::wstring path = fullName->AsString();

    std::size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos && pos != path.length() - 1)
        path = path.substr(pos + 1);

    return context.Collector.FromValue(path);
}

static ObjectInstance* shard_directory_Exists_get(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

    FieldSymbol* field = ownerType->Fields[0];
    ObjectInstance* fullName = instance->GetField(field);

    bool exists = fs::exists(fullName->AsString());
    return context.Collector.FromValue(exists);
}

static ObjectInstance* shard_directory_Create(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

    FieldSymbol* field = ownerType->Fields[0];
    ObjectInstance* fullName = instance->GetField(field);

    if (fs::exists(fullName->AsString()))
        return nullptr; // void

    if (!fs::create_directories(fullName->AsString()))
        throw std::runtime_error("Failed to create directory.");

    return nullptr; // void
}

static ObjectInstance* shard_directory_Delete(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    TypeSymbol* ownerType = const_cast<TypeSymbol*>(instance->getInfo());

    FieldSymbol* field = ownerType->Fields[0];
    ObjectInstance* fullName = instance->GetField(field);

    if (!fs::remove(fullName->AsString()))
        throw std::runtime_error("Failed to delete directory.");

    return nullptr; // void
}

static ObjectInstance* shard_directory_Exists(const CallState& context) noexcept(false)
{
    std::wstring path = context.Args[0]->AsString();
    bool exists = fs::exists(path);
    return context.Collector.FromValue(exists);
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
    instance->SetField(field, context.Args[0]);
    return instance;
}

static ObjectInstance* shard_directory_DeleteStatic(const CallState& context) noexcept(false)
{
    std::wstring path = context.Args[0]->AsString();
    if (!fs::remove(path))
        throw std::runtime_error("Failed to delete directory.");

    return nullptr; // void
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.io";
    lib.Description = L"Native implementation of filesystem methods";
    lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> fsNamespace(context, L"filesystem");

    // --- class File ---
    SymbolBuilder<ClassSymbol> fileClass = fsNamespace.AddClass(L"File");
    
    fileClass.AddMethod(L"ReadAllText", TYPE_STRING, LINK_STATIC)
             .AddParameter(L"fileName", TYPE_STRING)
             .SetCallback(&shard_file_ReadAllText);
    
    fileClass.AddMethod(L"WriteAllText", TYPE_VOID, LINK_STATIC)
             .AddParameter(L"fileName", TYPE_STRING)
             .AddParameter(L"content", TYPE_STRING)
             .SetCallback(&shard_file_WriteAllText);
    
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
    
    // --- class DirectoryInfo ---
    SymbolBuilder<ClassSymbol> dirInfoClass = fsNamespace.AddClass(L"DirectoryInfo");

    dirInfoClass
        .AddField(L"FullName", TYPE_STRING, LINK_INSTANCE);
    
    dirInfoClass
        .AddMethod(L"GetDirectory", dirInfoClass.Get(), LINK_STATIC)
        .AddParameter(L"fullName", TYPE_STRING)
        .SetCallback(&shard_directory_GetDirectory);
    
    dirInfoClass
        .AddMethod(L"Create", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_directory_Create);
    
    dirInfoClass
        .AddMethod(L"Delete", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_directory_Delete);
    
    dirInfoClass
        .AddProperty(L"Name", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_directory_Name_get);
    
    dirInfoClass
        .AddProperty(L"Exists", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter()
            .SetCallback(&shard_directory_Exists_get);

    // --- class Directory ---
    SymbolBuilder<ClassSymbol> directoryClass = fsNamespace.AddClass(L"Directory");
    
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
        .SetCallback(&shard_directory_DeleteStatic);
}