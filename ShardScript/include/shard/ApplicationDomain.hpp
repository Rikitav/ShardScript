#pragma once
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <memory>

namespace shard
{
	class SHARD_API ApplicationDomain
	{
		std::unique_ptr<ProgramVirtualImage> virtualProgram;
		GarbageCollector garbageCollector;
		VirtualMachine virtualMachine;

	public:
		ApplicationDomain(std::unique_ptr<ProgramVirtualImage> program);
		~ApplicationDomain();

		ApplicationDomain(const ApplicationDomain&) = delete;
		ApplicationDomain& operator=(const ApplicationDomain&) = delete;

		ProgramVirtualImage& GetProgram();
		GarbageCollector& GetGarbageCollector();
		VirtualMachine& GetVirtualMachine();
	};
}