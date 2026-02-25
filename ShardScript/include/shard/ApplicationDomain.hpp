#pragma once
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

namespace shard
{
	class SHARD_API ApplicationDomain
	{
		ProgramVirtualImage* virtualProgram;
		GarbageCollector garbageCollector;
		VirtualMachine virtualMachine;

	public:
		ApplicationDomain(ProgramVirtualImage* program);
		~ApplicationDomain();

		ProgramVirtualImage& GetProgram();
		GarbageCollector& GetGarbageCollector();
		VirtualMachine& GetVirtualMachine();
	};
}