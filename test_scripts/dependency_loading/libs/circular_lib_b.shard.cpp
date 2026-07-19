#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

using namespace shard;

SHARDLIB_GETMETADATA
{
    lib.Name = L"test.circular.b";
    lib.Description = L"Circular dependency test library B";
    lib.Version = L"1.0.0";

    static const shard::ShardLibDependencyInfo deps[] =
    {
        { L"test.circular.a", L"1.0.0" }
    };
    lib.Dependencies = deps;
    lib.DependenciesLength = sizeof(deps) / sizeof(deps[0]);
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> ns(context, L"circular_b");
    ns.AddClass(L"B");
}
