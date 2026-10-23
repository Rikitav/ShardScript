#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Containers.hpp>

#include <shard/compilation/VirtualProgramImage.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <memory>

namespace shard
{
	class SemanticModel;
	class PragmaticModel;

	class SHARD_API ApplicationDomain
	{
		gmt::unique_ptr<VirtualProgramImage> m_program;
		SemanticModel& m_model;
		PragmaticModel& m_pragmatic;
		VirtualMachine m_machine;

	public:
		ApplicationDomain(gmt::unique_ptr<VirtualProgramImage> program, SemanticModel& model, PragmaticModel& pragmatic);
		~ApplicationDomain();

		ApplicationDomain(const ApplicationDomain&) = delete;
		ApplicationDomain& operator=(const ApplicationDomain&) = delete;

		VirtualProgramImage& get_program() const;
		SemanticModel& get_semantic_model() const;
		PragmaticModel& get_pragmatic_model() const;
		VirtualMachine& get_virtual_machine();
	};
}
