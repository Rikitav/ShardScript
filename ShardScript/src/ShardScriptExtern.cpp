#if !defined(SHARDSCRIPT_STATIC)

#include <shard/ShardScriptAPI.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/ApplicationDomain.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>
#include <shard/parsing/lexical/reading/FileReader.hpp>

#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MemberSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>

#include <shard/syntax/nodes/CompilationUnitSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>
#include <shard/compilation/ProgramVirtualImage.hpp>

#include <sstream>
#include <filesystem>
#include <wchar.h>
#include <algorithm>
#include <string>

#ifdef __WIN32
#include <Windows.h>
#endif

#define SHARD_EXPORT extern "C" SHARD_API

using namespace shard;

namespace
{
    thread_local std::wstring LastErrorMessage;

    static void SetLastError(const std::string& message)
    {
        LastErrorMessage = std::wstring(message.begin(), message.end());
    }

    static void SetLastError(const std::wstring& message)
    {
        LastErrorMessage = message;
    }

    static void SetLastErrorFromException(const std::exception& ex)
    {
        SetLastError(ex.what());
    }
}

// =========================================================================
// Error Handling
// =========================================================================

SHARD_EXPORT int Shard_GetLastError(wchar_t* buffer, int bufferLen)
{
    if (buffer != nullptr && bufferLen > 0)
    {
        size_t copyLen = (std::min)((size_t)bufferLen - 1, LastErrorMessage.length());
        wcsncpy(buffer, LastErrorMessage.c_str(), copyLen);
        buffer[copyLen] = L'\0';
    }

    return (int)LastErrorMessage.length();
}

// =========================================================================
// Compilation Context API
// =========================================================================

SHARD_EXPORT CompilationContext* Shard_CreateCompilationContext()
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

SHARD_EXPORT int Shard_DestroyCompilationContext(CompilationContext* ctx)
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

SHARD_EXPORT int Shard_AddLibrary(CompilationContext* ctx, const wchar_t* path)
{
    try
    {
        if (ctx == nullptr)
        {
            SetLastError(L"compilation context is null");
            return -1;
        }

        if (path == nullptr)
        {
            SetLastError(L"library path is null");
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

SHARD_EXPORT int Shard_AddSource(CompilationContext* ctx, const wchar_t* sourceName, const wchar_t* code, CompilationUnitOrigin origin)
{
    try
    {
        if (ctx == nullptr || code == nullptr || sourceName == nullptr)
        {
            SetLastError(L"invalid argument");
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

SHARD_EXPORT int Shard_AddSourceFile(CompilationContext* ctx, const wchar_t* filePath, CompilationUnitOrigin origin)
{
    try
    {
        if (ctx == nullptr || filePath == nullptr)
        {
            SetLastError(L"invalid argument");
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

SHARD_EXPORT int Shard_Analyze(CompilationContext* ctx)
{
    try
    {
        if (ctx == nullptr)
        {
            SetLastError(L"compilation context is null");
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

SHARD_EXPORT ApplicationDomain* Shard_Compile(CompilationContext* ctx)
{
    try
    {
        if (ctx == nullptr)
        {
            SetLastError(L"compilation context is null");
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

SHARD_EXPORT ApplicationDomain* Shard_CompileAndRun(CompilationContext* ctx)
{
    try
    {
        ApplicationDomain* domain = Shard_Compile(ctx);
        if (domain != nullptr)
            domain->GetVirtualMachine().Run();

        return domain;
    }
    catch (const std::exception& e)
    {
        SetLastErrorFromException(e);
        return nullptr;
    }
}

SHARD_EXPORT int Shard_SetEntryPoint(CompilationContext* ctx, int value)
{
    try
    {
        if (ctx == nullptr)
        {
            SetLastError(L"compilation context is null");
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

SHARD_EXPORT int Shard_GetEntryPoint(CompilationContext* ctx)
{
    try
    {
        if (ctx == nullptr)
        {
            SetLastError(L"compilation context is null");
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

SHARD_EXPORT int Shard_HasErrors(CompilationContext* ctx)
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

SHARD_EXPORT int Shard_GetErrorCount(CompilationContext* ctx)
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

SHARD_EXPORT int Shard_ResetDiagnostics(CompilationContext* ctx)
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

SHARD_EXPORT int Shard_GetDiagnostics(CompilationContext* ctx, wchar_t* buffer, int bufferLen)
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
            size_t copyLen = (std::min)((size_t)bufferLen - 1, str.length());
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

SHARD_EXPORT int Shard_RunDomain(ApplicationDomain* domain)
{
    try
    {
        if (domain == nullptr)
        {
            SetLastError(L"domain is null");
            return -1;
        }

        domain->GetVirtualMachine().Run();
        return 0;
    }
    catch (const std::exception& e)
    {
        SetLastErrorFromException(e);
        return -1;
    }
}

SHARD_EXPORT int Shard_DestroyDomain(ApplicationDomain* domain)
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

SHARD_EXPORT VirtualMachine* Shard_GetVirtualMachine(ApplicationDomain* domain)
{
    try
    {
        if (domain == nullptr)
        {
            SetLastError(L"domain is null");
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

SHARD_EXPORT GarbageCollector* Shard_GetGarbageCollector(ApplicationDomain* domain)
{
    try
    {
        if (domain == nullptr)
        {
            SetLastError(L"domain is null");
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

SHARD_EXPORT ProgramVirtualImage* Shard_GetProgram(ApplicationDomain* domain)
{
    try
    {
        if (domain == nullptr)
        {
            SetLastError(L"domain is null");
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

SHARD_EXPORT MethodSymbol* Shard_GetEntryPointMethod(ApplicationDomain* domain)
{
    try
    {
        if (domain == nullptr)
        {
            SetLastError(L"domain is null");
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

// =========================================================================
// Virtual Machine API
// =========================================================================

SHARD_EXPORT int Shard_VMRun(VirtualMachine* vm)
{
    try
    {
        if (vm == nullptr)
        {
            SetLastError(L"virtual machine is null");
            return -1;
        }

        vm->Run();
        return 0;
    }
    catch (const std::exception& e)
    {
        SetLastErrorFromException(e);
        return -1;
    }
}

SHARD_EXPORT int Shard_VMAbort(VirtualMachine* vm)
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

SHARD_EXPORT int Shard_VMTerminateCallStack(VirtualMachine* vm)
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

SHARD_EXPORT ObjectInstance* Shard_VMInvokeMethod(VirtualMachine* vm, MethodSymbol* method, ObjectInstance** args, int argCount)
{
    try
    {
        if (vm == nullptr)
        {
            SetLastError(L"virtual machine is null");
            return nullptr;
        }

        if (method == nullptr)
        {
            SetLastError(L"method is null");
            return nullptr;
        }

        return vm->InvokeMethod(method, args, static_cast<size_t>(argCount));
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

SHARD_EXPORT ObjectInstance* Shard_GCFromInteger(GarbageCollector* gc, int64_t value)
{
    try
    {
        if (gc == nullptr)
        {
            SetLastError(L"garbage collector is null");
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

SHARD_EXPORT ObjectInstance* Shard_GCFromDouble(GarbageCollector* gc, double value)
{
    try
    {
        if (gc == nullptr)
        {
            SetLastError(L"garbage collector is null");
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

SHARD_EXPORT ObjectInstance* Shard_GCFromBool(GarbageCollector* gc, int value)
{
    try
    {
        if (gc == nullptr)
        {
            SetLastError(L"garbage collector is null");
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

SHARD_EXPORT ObjectInstance* Shard_GCFromString(GarbageCollector* gc, const wchar_t* value)
{
    try
    {
        if (gc == nullptr || value == nullptr)
        {
            SetLastError(L"garbage collector or value is null");
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

SHARD_EXPORT int64_t Shard_ReadInteger(ObjectInstance* instance)
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

SHARD_EXPORT double Shard_ReadDouble(ObjectInstance* instance)
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

SHARD_EXPORT int Shard_ReadBool(ObjectInstance* instance)
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

SHARD_EXPORT const wchar_t* Shard_ReadString(ObjectInstance* instance)
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
// Symbol Inspection API
// =========================================================================

SHARD_EXPORT int Shard_GetCompilationUnitCount(CompilationContext* ctx)
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

SHARD_EXPORT CompilationUnitSyntax* Shard_GetCompilationUnit(CompilationContext* ctx, int index)
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

SHARD_EXPORT int Shard_GetCompilationUnitOrigin(CompilationUnitSyntax* unit)
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

SHARD_EXPORT NamespaceDeclarationSyntax* Shard_GetUnitNamespace(CompilationUnitSyntax* unit)
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

SHARD_EXPORT int Shard_GetNamespaceIdentifierCount(NamespaceDeclarationSyntax* ns)
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

SHARD_EXPORT const wchar_t* Shard_GetNamespaceIdentifier(NamespaceDeclarationSyntax* ns, int index)
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

SHARD_EXPORT int Shard_GetUnitClassCount(CompilationUnitSyntax* unit)
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

SHARD_EXPORT ClassDeclarationSyntax* Shard_GetUnitClass(CompilationUnitSyntax* unit, int index)
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

SHARD_EXPORT const wchar_t* Shard_GetTypeName(TypeDeclarationSyntax* type)
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

SHARD_EXPORT int Shard_GetTypeMethodCount(CompilationContext* ctx, TypeDeclarationSyntax* type)
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

SHARD_EXPORT MethodSymbol* Shard_GetTypeMethod(CompilationContext* ctx, TypeDeclarationSyntax* type, int index)
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

SHARD_EXPORT int Shard_GetSymbolTableTypeCount(CompilationContext* ctx)
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

SHARD_EXPORT TypeSymbol* Shard_GetSymbolTableType(CompilationContext* ctx, int index)
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

SHARD_EXPORT TypeSymbol* Shard_FindType(CompilationContext* ctx, const wchar_t* name)
{
    try
    {
        if (ctx == nullptr || name == nullptr)
            return nullptr;

        auto types = ctx->GetSemanticModel().Table->GetTypeSymbols();
        for (TypeSymbol* type : types)
        {
            if (type != nullptr && type->Name == name)
                return type;
        }

        return nullptr;
    }
    catch (const std::exception& e)
    {
        SetLastErrorFromException(e);
        return nullptr;
    }
}

SHARD_EXPORT MethodSymbol* Shard_FindMethodInType(TypeSymbol* type, const wchar_t* name, int parameterCount)
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

SHARD_EXPORT const wchar_t* Shard_GetSymbolName(SyntaxSymbol* symbol)
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

SHARD_EXPORT const wchar_t* Shard_GetMethodName(MethodSymbol* method)
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

SHARD_EXPORT int Shard_GetMethodParameterCount(MethodSymbol* method)
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

SHARD_EXPORT const wchar_t* Shard_GetMethodParameterName(MethodSymbol* method, int index)
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

SHARD_EXPORT TypeSymbol* Shard_GetMethodParameterType(MethodSymbol* method, int index)
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

SHARD_EXPORT TypeSymbol* Shard_GetMethodReturnType(MethodSymbol* method)
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

SHARD_EXPORT int Shard_IsMethodStatic(MethodSymbol* method)
{
    try
    {
        if (method == nullptr)
            return 0;

        return method->IsStatic ? 1 : 0;
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

SHARD_EXPORT const wchar_t* Shard_GetVersion()
{
    return L"0.2.0";
}

#endif // !defined(SHARDSCRIPT_STATIC)