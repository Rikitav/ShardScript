#include <shard/ApplicationDomain.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(ProgramVirtualImage* program) : virtualProgram(program), virtualMachine(this), garbageCollector(this)
{

}

ApplicationDomain::~ApplicationDomain()
{
	if (virtualProgram != nullptr)
		delete virtualProgram;
}

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
