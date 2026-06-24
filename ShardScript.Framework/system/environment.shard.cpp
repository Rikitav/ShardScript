#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolBuilder.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <cstdlib>
#include <string>

using namespace shard;

static ObjectInstance* shard_environment_GetEnv(const CallState& context)
{
    const wchar_t* name = context.Args[0]->AsString();
    const wchar_t* value = _wgetenv(name);
    if (value == nullptr)
        value = L"";

    return context.Collector.FromValue(std::wstring(value));
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
