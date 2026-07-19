#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

using namespace shard;

SHARDLIB_GETMETADATA
{
    lib.Name = L"test.circular.a";
    lib.Description = L"Circular dependency test library A";
    lib.Version = L"1.0.0";

    static const shard::ShardLibDependencyInfo deps[] =
    {
        { L"test.circular.b", L"1.0.0" }
    };
    lib.Dependencies = deps;
    lib.DependenciesLength = sizeof(deps) / sizeof(deps[0]);
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> ns(context, L"circular_a");
    ns.AddClass(L"A");
}
