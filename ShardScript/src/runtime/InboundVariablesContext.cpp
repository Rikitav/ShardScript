#include <shard/runtime/InboundVariablesContext.h>

using namespace std;
using namespace shard::runtime;

shared_ptr<VariableRegister> InboundVariablesContext::TryFind(string name)
{
	if (auto search = Heap.find(name); search != Heap.end())
		return search->second;

	return Previous == nullptr ? nullptr : Previous->TryFind(name);
}

InboundVariablesContext::~InboundVariablesContext()
{
	Heap.clear();
}
