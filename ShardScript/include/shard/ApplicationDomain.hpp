#pragma once
#include <shard/Definitions.hpp>

#include <shard/compilation/VirtualProgramImage.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <memory>

namespace shard
{
	class SemanticModel;
	class PragmaticModel;

	class SHARD_API ApplicationDomain
	{
		std::unique_ptr<VirtualProgramImage> m_program;
		VirtualMachine m_machine;

	public:
		ApplicationDomain(std::unique_ptr<VirtualProgramImage> program, SemanticModel& model, PragmaticModel& pragmatic);
		~ApplicationDomain() = default;

		ApplicationDomain(const ApplicationDomain&) = delete;
		ApplicationDomain& operator=(const ApplicationDomain&) = delete;

		VirtualProgramImage& get_program() const;
		VirtualMachine& get_virtual_machine();
	};
}
