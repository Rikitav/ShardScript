#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>
#include <shard/runtime/MethodCallState.hpp>

using namespace shard;

void SymbolTable::ResolveInterfaces(SymbolTable* globalTable)
{
	SymbolFactory factory(globalTable);

	// IThrowable
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IThrowable", SymbolTable::Global::Namespace);
		TRAIT_THROWABLE = builder
			.DeclareGlobal();

		TRAIT_THROWABLE_getMessage = builder.AddProperty(L"message", TYPE_STRING, LINK_INSTANCE).AddGetter();
		TRAIT_THROWABLE_getStackTrace = builder.AddProperty(L"stack_trace", TYPE_STRING, LINK_INSTANCE).AddGetter();

		TRAIT_THROWABLE_getMessage->IsAbstract = true;
		TRAIT_THROWABLE_getStackTrace->IsAbstract = true;
	}

	// IDisposable
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IDisposable", SymbolTable::Global::Namespace);
		TRAIT_DISPOSABLE = builder
			.DeclareGlobal();

		TRAIT_DISPOSABLE_Dispose = builder.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE);
		TRAIT_DISPOSABLE_Dispose->IsAbstract = true;
	}

	// IPrintable
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IPrintable", SymbolTable::Global::Namespace);
		TRAIT_PRINTABLE = builder
			.DeclareGlobal();

		TRAIT_PRINTABLE_ToString = builder.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE);
		TRAIT_PRINTABLE_ToString->IsAbstract = true;
	}

	// IEnumerator<T>
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IEnumerator", SymbolTable::Global::Namespace);
		TRAIT_ENUMERATOR = builder
			.DeclareGlobal();

		TypeParameterSymbol* enumeratorTypeParam = builder
			.AddTypeParameter(L"T");

		TRAIT_ENUMERATOR_MOVENEXT = builder.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE);
		TRAIT_ENUMERATOR_CURRENT_GET = builder.AddProperty(L"Current", enumeratorTypeParam, LINK_INSTANCE).AddGetter();

		TRAIT_ENUMERATOR_MOVENEXT->IsAbstract = true;
		TRAIT_ENUMERATOR_CURRENT_GET->IsAbstract = true;
	}

	// IEnumerable<T>
	{
		SymbolBuilder<InterfaceSymbol> builder(globalTable, L"IEnumerable", SymbolTable::Global::Namespace);
		TRAIT_ENUMERABLE = builder
			.DeclareGlobal();

		TypeParameterSymbol* enumerableTypeParam = builder
			.AddTypeParameter(L"T");

		GenericTypeSymbol* enumeratorReturnType = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", enumerableTypeParam } });
		TRAIT_ENUMERABLE_GETENUMERATOR = builder.AddMethod(L"GetEnumerator", enumeratorReturnType, LINK_INSTANCE);
		TRAIT_ENUMERABLE_GETENUMERATOR->IsAbstract = true;
	}
}
