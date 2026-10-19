#include <shard/ApplicationDomain.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(std::unique_ptr<VirtualProgramImage> program, SemanticModel& model, PragmaticModel& pragmatic)
	: m_program(std::move(program))
	, m_machine(*m_program, model, pragmatic)
{ }

VirtualProgramImage& ApplicationDomain::get_program() const
{
	return *m_program;
}

VirtualMachine& ApplicationDomain::get_virtual_machine()
{
	return m_machine;
}
