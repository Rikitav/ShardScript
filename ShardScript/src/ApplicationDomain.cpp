#include <shard/ApplicationDomain.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(gmt::unique_ptr<VirtualProgramImage> program, SemanticModel& model, PragmaticModel& pragmatic)
	: m_program(std::move(program))
	, m_model(model)
	, m_pragmatic(pragmatic)
	, m_machine(*m_program, model, pragmatic)
{ }

ApplicationDomain::~ApplicationDomain() = default;

VirtualProgramImage& ApplicationDomain::get_program() const
{
	return *m_program;
}

SemanticModel& ApplicationDomain::get_semantic_model() const
{
	return m_model;
}

PragmaticModel& ApplicationDomain::get_pragmatic_model() const
{
	return m_pragmatic;
}

VirtualMachine& ApplicationDomain::get_virtual_machine()
{
	return m_machine;
}
