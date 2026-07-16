#include <shard/parsing/SyntaxFacts.hpp>

#include <shard/lexical/TokenType.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolFactory.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace shard;

void SymbolTable::ResolveGlobalMethods(SymbolTable* globalTable)
{
	SymbolFactory factory(globalTable);

	MethodSymbol* haltMethod = factory.Method(ACS_PUBLIC, LINK_STATIC, TYPE_VOID, L"Halt", [](const CallState& context) -> ObjectInstance*
	{
		context.Runtimer.Abort();
		return nullptr;
	});

	SymbolTable::Global::Namespace->OnSymbolDeclared(haltMethod);
}
