#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <string>
#include <stdexcept>

using namespace shard::runtime;

ObjectInstance* InboundVariablesContext::AddVariable(const std::wstring name, ObjectInstance* instance)
{
	if (TryFind(name))
		throw std::runtime_error("variable already created");

	return Variables[name] = GarbageCollector::CopyInstance(instance);
}

ObjectInstance* InboundVariablesContext::TryFind(const std::wstring& name)
{
	if (auto search = Variables.find(name); search != Variables.end())
		return search->second;

	if (Previous == nullptr)
		return nullptr;

	return ((InboundVariablesContext*)Previous)->TryFind(name);
}

InboundVariablesContext::~InboundVariablesContext()
{
	if (Variables.empty())
		return;

	for (auto const& instance : Variables)
		GarbageCollector::DestroyInstance(instance.second);

	Variables.clear();
}
