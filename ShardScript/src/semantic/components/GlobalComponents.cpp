#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <memory>

using namespace shard;

static std::unique_ptr<SymbolTable> GlobalSymbolTable;
static bool CreatingGlobalSymbolTable = false;

void SymbolTable::ResolveGlobalComponents(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved || CreatingGlobalSymbolTable)
		return;

	// Standard components are owned by a process-wide table so that their
	// pointers remain valid after any individual CompilationContext is destroyed.
	CreatingGlobalSymbolTable = true;
	GlobalSymbolTable = std::make_unique<SymbolTable>();
	CreatingGlobalSymbolTable = false;

	SymbolTable* global = GlobalSymbolTable.get();
	SymbolTable::Global::Namespace->Node = new NamespaceNode();

	// Resolve standard interface traits before they are used below.
	ResolvePrimitives(global);
	ResolveInterfaces(global);

	// Primitives
	ResolvePrimitiveOperators(global);
	ResolvePrimitivePrintables(global);

	ResolveEnumerables(global);
	ResolveExceptions(global);
	ResolveAsyncTypes(global);
	ResolveGlobalMethods(global);

	global->MarkAllSymbolsReady();
	resolved = true;
}
