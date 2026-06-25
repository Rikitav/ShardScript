#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>

using namespace shard;

static ObjectInstance* shard_environment_GetEnv(const CallState& context)
{
    const wchar_t* name = context.Args[0]->AsString();
#ifdef _WIN32
    const wchar_t* value = _wgetenv(name);
    return context.Collector.FromValue(std::wstring(value ? value : L""));
#else
    std::string narrowName(name, name + std::wcslen(name));
    const char* value = std::getenv(narrowName.c_str());
    if (value == nullptr)
        value = "";
    return context.Collector.FromValue(std::wstring(value, value + std::strlen(value)));
#endif
}

SHARDLIB_GETMETADATA
{
    lib.Name        = L"environment";
    lib.Description = L"Access environment variables via static access operator";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    auto envNamespace = SymbolBuilder<NamespaceSymbol>(context, L"environment");
    auto envClass = envNamespace.AddClass(L"Environment");
    envClass.Get()->Linking = LINK_STATIC;

    envClass.AddOperator(shard::TokenType::Delimeter, SymbolTable::Primitives::String, LINK_STATIC)
        .AddParameter(L"name", SymbolTable::Primitives::String)
        .SetCallback(&shard_environment_GetEnv);
}
