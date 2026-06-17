#include <ShardScript.hpp>
#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>
#include <iostream>

using namespace shard;

static ObjectInstance* shard_debug_typeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return context.Collector.FromValue(instance->getInfo()->Name);
}

static ObjectInstance* shard_debug_sizeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return context.Collector.FromValue(static_cast<std::int64_t>(instance->getInfo()->MemoryBytesSize));
}

static ObjectInstance* shard_debug_PrintGcInfo(const CallState& context)
{
	std::wcout << "\nGarbage collector info dump" << std::endl;
	for (ObjectInstance* reg : context.Collector.Heap)
	{
		std::wcout
			<< L" * PTR : " << reg->getMemory()
			<< L" | TYPE : '" << reg->getInfo()->Name << "'"
			<< L" | REFS : " << reg->getReferencesCounter() << "\n";
	}

	std::wcout << "Total count : " << context.Collector.Heap.size() << std::endl;
	return nullptr; // void
}

static ObjectInstance* shard_debug_PrintStackFrameInfo(const CallState& context)
{
	std::wcout << "\nCall stack frame variables dump :" << std::endl;

	CallStackFrame* frame = context.Runtimer.CurrentFrame()->PreviousFrame;
	for (int i = 0; i < context.Method->GetEvalStackLocalsCount(); i++)
	{
		ObjectInstance* reg = frame->EvalStack[i];
		std::wcout
			<< L" * PTR : " << reg->getMemory()
			<< L" | TYPE : '" << reg->getInfo()->Name << "'"
			<< L" | REFS : " << reg->getReferencesCounter() << "\n";
	}

	std::wcout << "Total count : " << context.Method->GetEvalStackLocalsCount() << std::endl;
	return nullptr; // void
}

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.debug";
	lib.Description = L"ShardScript debug and inspection functions";
	lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> debug(context, L"debug");

	debug.AddMethod(L"typeof", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"object", TYPE_ANY)
		 .SetCallback(&shard_debug_typeof);

	debug.AddMethod(L"sizeof", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"object", TYPE_ANY)
		 .SetCallback(&shard_debug_sizeof);

	debug.AddMethod(L"PrintGcInfo", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		 .SetCallback(&shard_debug_PrintGcInfo);

	debug.AddMethod(L"PrintGcInfo", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.SetCallback(&shard_debug_PrintGcInfo);
}
