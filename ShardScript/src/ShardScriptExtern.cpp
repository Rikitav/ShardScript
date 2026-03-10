#include <shard/ShardScriptAPI.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/ApplicationDomain.hpp>

#include <shard/parsing/lexical/LexicalAnalyzer.hpp>
#include <shard/parsing/lexical/reading/StringStreamReader.hpp>

#include <sstream>
#include <algorithm>
#include <filesystem>

#define SHARD_EXPORT extern "C" SHARD_API

using namespace shard;

// =========================================================================
// Compilation Context API
// =========================================================================

SHARD_EXPORT CompilationContext* Shard_CreateCompilationContext()
{
    return new CompilationContext();
}

SHARD_EXPORT void Shard_DestroyCompilationContext(CompilationContext* ctx)
{
    if (ctx != nullptr)
        delete ctx;
}

SHARD_EXPORT void Shard_AddLibrary(CompilationContext* ctx, const wchar_t* path)
{
    if (ctx != nullptr && path != nullptr)
        ctx->AddLib(std::filesystem::path(path));
}

SHARD_EXPORT void Shard_AddSource(CompilationContext* ctx, const wchar_t* sourceName, const wchar_t* code)
{
    if (ctx == nullptr || code == nullptr || sourceName == nullptr)
        return;

    std::wstring sourceCode(code);
    StringStreamReader reader(sourceName, sourceCode);
    LexicalAnalyzer lexer(reader);

    ctx->EnrichTree(lexer);
}

SHARD_EXPORT void Shard_Analyze(CompilationContext* ctx)
{
    if (ctx != nullptr)
        ctx->AnalyzeTree();
}

SHARD_EXPORT ApplicationDomain* Shard_Compile(CompilationContext* ctx)
{
    if (ctx == nullptr)
        return nullptr;

    return ctx->Compile();
}

SHARD_EXPORT bool Shard_HasErrors(CompilationContext* ctx)
{
    if (ctx == nullptr)
        return true;

    return ctx->GetDiagnosticsContext().AnyError;
}

SHARD_EXPORT int Shard_GetDiagnostics(CompilationContext* ctx, wchar_t* buffer, int bufferLen)
{
    if (ctx == nullptr) return 0;

    std::wstringstream ss;
    ctx->GetDiagnosticsContext().WriteDiagnostics(ss);
    std::wstring str = ss.str();

    if (buffer != nullptr && bufferLen > 0)
    {
        size_t copyLen = std::min((size_t)bufferLen - 1, str.length());
        wcsncpy(buffer, str.c_str(), copyLen);
        buffer[copyLen] = L'\0';
    }

    return (int)str.length();
}

// =========================================================================
// Application Domain API
// =========================================================================

SHARD_EXPORT void Shard_RunDomain(ApplicationDomain* domain)
{
    if (domain != nullptr)
        domain->GetVirtualMachine().Run();
}

SHARD_EXPORT void Shard_DestroyDomain(ApplicationDomain* domain)
{
    if (domain != nullptr)
        delete domain;
}