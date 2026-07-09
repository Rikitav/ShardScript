#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace shard;

TypeSymbol* shard_FileInfo = nullptr;
FieldSymbol* shard_FileInfo_FullNameBackingField = nullptr;

TypeSymbol* shard_DirectoryInfo = nullptr;
FieldSymbol* shard_DirectoryInfo_FullNameBackingField = nullptr;

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

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.io";
    lib.Description = L"Native implementation of filesystem methods";
    lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> fsNamespace(context, L"filesystem");

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