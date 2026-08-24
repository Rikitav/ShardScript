#pragma once
#include <shard/compilation/ProgramVirtualImage.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <memory>
#include <string>
#include <vector>

namespace shard
{
	class EventLoop;

	class SHARD_API ApplicationDomain
	{
		std::unique_ptr<ProgramVirtualImage> virtualProgram;
		GarbageCollector garbageCollector;
		std::unique_ptr<EventLoop> eventLoop;
		VirtualMachine virtualMachine;
		std::vector<std::wstring> scriptArguments;

	public:
		ApplicationDomain(std::unique_ptr<ProgramVirtualImage> program);
		~ApplicationDomain();

		ApplicationDomain(const ApplicationDomain&) = delete;
		ApplicationDomain& operator=(const ApplicationDomain&) = delete;

		ProgramVirtualImage& GetProgram();
		EventLoop& GetEventLoop();
		GarbageCollector& GetGarbageCollector();
		VirtualMachine& GetVirtualMachine();

		inline void SetScriptArguments(const std::vector<std::wstring>& args) { scriptArguments = args; }
		inline void SetScriptArguments(std::vector<std::wstring>&& args) { scriptArguments = std::move(args); }
		inline const std::vector<std::wstring>& GetScriptArguments() const { return scriptArguments; }
	};
}