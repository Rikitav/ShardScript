#include <shard/ApplicationDomain.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(gmt::unique_ptr<VirtualProgramImage> program, gmt::shared_ptr<CompilationState> state)
	: m_state(std::move(state))
	, m_program(std::move(program))
	, m_machine(*m_program, m_state->m_model, m_state->m_pragmatic)
{ }

ApplicationDomain::~ApplicationDomain() = default;

VirtualProgramImage& ApplicationDomain::get_program() const
{
	return *m_program;
}

SemanticModel& ApplicationDomain::get_semantic_model() const
{
	return m_state->m_model;
}

PragmaticModel& ApplicationDomain::get_pragmatic_model() const
{
	return m_state->m_pragmatic;
}

VirtualMachine& ApplicationDomain::get_virtual_machine()
{
	return m_machine;
}
