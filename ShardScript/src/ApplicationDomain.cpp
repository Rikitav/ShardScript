#include <shard/ApplicationDomain.hpp>
#include <shard/runtime/EventLoop.hpp>

using namespace shard;

ApplicationDomain::ApplicationDomain(std::unique_ptr<ProgramVirtualImage> program)
	: virtualProgram(std::move(program)), garbageCollector(this), eventLoop(std::make_unique<EventLoop>()), virtualMachine(this)
{

}

ApplicationDomain::~ApplicationDomain() = default;

ProgramVirtualImage& ApplicationDomain::GetProgram()
{
	return *virtualProgram;
}

EventLoop& ApplicationDomain::GetEventLoop()
{
	return *eventLoop;
}

GarbageCollector& ApplicationDomain::GetGarbageCollector()
{
	return garbageCollector;
}

VirtualMachine& ApplicationDomain::GetVirtualMachine()
{
	return virtualMachine;
}
