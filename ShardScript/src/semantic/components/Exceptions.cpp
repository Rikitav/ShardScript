#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/semantic/symbols/FieldSymbol.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>

using namespace shard;

namespace
{
	static inline void inherit_size(FieldSymbol* field)
	{
		TypeSymbol* parent = static_cast<TypeSymbol*>(field->Parent);
		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			field->SlotIndex = parent->NextSlotIndex++;

		field->MemoryBytesOffset = parent->MemoryBytesSize;
		parent->MemoryBytesSize += field->ReturnType->GetInlineSize();
	}
}

static ObjectInstance* runtime_capture_stack_trace(const CallState& context)
{
	std::wstring trace = context.Runtimer.GetStackTrace();
	return context.Collector.FromValue(trace);
}

void SymbolTable::ResolveExceptions(SymbolTable* table)
{
	static bool resolved = false;
	if (resolved)
		return;

	SymbolFactory factory(table);

	// Runtime class with capture_stack_trace
	{
		SymbolBuilder<ClassSymbol> builder = SymbolBuilder<ClassSymbol>(table, L"runtime", SymbolTable::Global::Namespace);
		SymbolTable::StandardTypes::Runtime = builder
			.DeclareGlobal();

		SymbolTable::StandardTypes::RuntimeCaptureStackTrace = builder.AddMethod(L"capture_stack_trace", SymbolTable::Primitives::String, LINK_STATIC)
			.SetCallback(&runtime_capture_stack_trace);
	}

	// RuntimeException class
	{
		SymbolBuilder<ClassSymbol> builder = SymbolBuilder<ClassSymbol>(table, L"RuntimeException", SymbolTable::Global::Namespace);
		SymbolTable::StandardTypes::RuntimeException = builder
			.Implements(TRAIT_THROWABLE)
			.DeclareGlobal();

		builder
			.AddInit();

		SymbolBuilder<PropertySymbol> messageProp = builder.AddProperty(L"message", SymbolTable::Primitives::String, LINK_INSTANCE);
		SymbolTable::StandardTypes::RuntimeExceptionMessageField = messageProp
			.AddBackingField();

		messageProp.AddGetter()
			.IsImplementationOf(TRAIT_THROWABLE_getMessage)
			.SetCallback([](const CallState& context) { return context.Args[0]->GetField(SymbolTable::StandardTypes::RuntimeExceptionMessageField->SlotIndex); });

		SymbolBuilder<PropertySymbol> stackTraceProp = builder.AddProperty(L"message", SymbolTable::Primitives::String, LINK_INSTANCE);
		SymbolTable::StandardTypes::RuntimeExceptionStackTraceField = stackTraceProp
			.AddBackingField();

		stackTraceProp.AddGetter()
			.IsImplementationOf(TRAIT_THROWABLE_getStackTrace)
			.SetCallback([](const CallState& context) { return context.Args[0]->GetField(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField->SlotIndex); });

		// RuntimeException is created implicitly and does not go through normal
		// layout generation; lay it out manually so the runtime can instantiate it.
		inherit_size(SymbolTable::StandardTypes::RuntimeExceptionMessageField);
		inherit_size(SymbolTable::StandardTypes::RuntimeExceptionStackTraceField);

		SymbolTable::StandardTypes::RuntimeException->AnalysisState = SymbolAnalysisState::Ready;
		SymbolTable::StandardTypes::RuntimeException->LayoutingState = TypeLayoutingState::Visited;
	}

	resolved = true;
}
