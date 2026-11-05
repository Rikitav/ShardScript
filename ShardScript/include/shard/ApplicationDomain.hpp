#pragma once
#include <shard/Definitions.hpp>
#include <gmt/Containers.hpp>
#include <shard/CompilationState.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>
#include <shard/runtime/VirtualMachine.hpp>

namespace shard
{
	class SHARD_API ApplicationDomain
	{
		gmt::shared_ptr<CompilationState> m_state;
		gmt::unique_ptr<VirtualProgramImage> m_program;
		VirtualMachine m_machine;

	public:
		ApplicationDomain(gmt::unique_ptr<VirtualProgramImage> program, gmt::shared_ptr<CompilationState> state);
		~ApplicationDomain();

		ApplicationDomain(const ApplicationDomain&) = delete;
		ApplicationDomain& operator=(const ApplicationDomain&) = delete;

		VirtualProgramImage& get_program() const;
		SemanticModel& get_semantic_model() const;
		PragmaticModel& get_pragmatic_model() const;
		VirtualMachine& get_virtual_machine();
	};
}
