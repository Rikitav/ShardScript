#include <shard/ApplicationDomain.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(std::unique_ptr<ProgramVirtualImage> program)
	: virtualProgram(std::move(program)), virtualMachine(this), garbageCollector(this)
{

}

ApplicationDomain::~ApplicationDomain() = default;

ProgramVirtualImage& ApplicationDomain::GetProgram()
{
	return *virtualProgram;
}

GarbageCollector& ApplicationDomain::GetGarbageCollector()
{
	return garbageCollector;
}

VirtualMachine& ApplicationDomain::GetVirtualMachine()
{
	return virtualMachine;
}
