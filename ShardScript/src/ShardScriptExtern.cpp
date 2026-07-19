#if !defined(SHARDSCRIPT_STATIC)

#include <shard/ShardScriptAPI.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/ApplicationDomain.hpp>
#include <shard/parsing/SyntaxTree.hpp>

#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/StringStreamReader.hpp>
#include <shard/lexical/FileReader.hpp>

#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/NamespaceDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <shard/parsing/nodes/BodyDeclarationSyntax.hpp>
#include <shard/parsing/nodes/TypeDeclarationSyntax.hpp>
#include <shard/parsing/nodes/StatementsBlockSyntax.hpp>
#include <shard/parsing/nodes/ArgumentsListSyntax.hpp>
#include <shard/parsing/nodes/ParametersListSyntax.hpp>

#include <shard/parsing/nodes/MemberDeclarations/ClassDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/StructDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/ConstructorDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/FieldDeclarationSyntax.hpp>
#include <shard/parsing/nodes/MemberDeclarations/MethodDeclarationSyntax.hpp>

#include <shard/parsing/nodes/StatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/VariableStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/DeferStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/BreakStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/nodes/Statements/TryStatementSyntax.hpp>

#include <shard/parsing/nodes/Loops/ForEachStatementSyntax.hpp>
#include <shard/parsing/nodes/Loops/WhileStatementSyntax.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LinkedExpressionSyntax.hpp>

#include <shard/semantic/symbols/InterfaceSymbol.hpp>

#include <shard/ShardScriptExtern.hpp>

#include <shard/parsing/nodes/Types/PredefinedTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/ArrayTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/NullableTypeSyntax.hpp>
#include <shard/parsing/nodes/Types/GenericTypeSyntax.hpp>
#include <shard/parsing/nodes/TypeArgumentsListSyntax.hpp>

#include <shard/compilation/ProgramVirtualImage.hpp>

#include <sstream>
#include <filesystem>
#include <wchar.h>
#include <algorithm>
#include <string>
#include <unordered_map>

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

    static SyntaxToken MakeToken(shard::TokenType type, const wchar_t* word = nullptr)
    {
        return SyntaxToken(type, word != nullptr ? std::wstring(word) : std::wstring(), TextLocation(), false);
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
        return SHARDSCRIPT_VERSION;
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
            if (name == nullptr)
            {
                SetLastShardWError(L"name is null");
                return nullptr;
            }

            MemberDeclarationInfo info;
            info.Identifier = MakeToken(shard::TokenType::Identifier, name);
            info.ReturnType = std::make_unique<PredefinedTypeSyntax>(MakeToken(shard::TokenType::VoidKeyword, L"void"), nullptr);
            auto* decl = new ConstructorDeclarationSyntax(info, parent);
            decl->DeclareToken = MakeToken(shard::TokenType::InitKeyword, L"init");
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

            if (!symbol->IsMember())
            {
                SetLastShardWError(L"symbol does not support linking");
                return -1;
            }

            static_cast<MemberSymbol*>(symbol)->Linking = static_cast<SymbolLinking>(linking);
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
        int /*isStatic*/,
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
            PropertySymbol* property = factory.Property(name, type, LINK_INSTANCE);
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