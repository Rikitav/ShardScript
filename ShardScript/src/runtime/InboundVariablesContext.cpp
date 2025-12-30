#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/GarbageCollector.h>
#include <string>
#include <stdexcept>

using namespace shard;

ObjectInstance* InboundVariablesContext::AddVariable(const std::wstring name, ObjectInstance* instance)
{
	auto search = Variables.find(name);
	if (search != Variables.end())
		throw std::runtime_error("variable already exists");

	Variables[name] = GarbageCollector::CopyInstance(instance);
	return instance;
}

ObjectInstance* InboundVariablesContext::SetVariable(const std::wstring name, ObjectInstance* instance)
{
	auto search = Variables.find(name);
	if (search != Variables.end())
	{
		GarbageCollector::DestroyInstance(search->second);

		Variables[name] = GarbageCollector::CopyInstance(instance);
		return instance;
	}
	else
	{
		if (Previous == nullptr)
			throw std::runtime_error("variable not found");

		return const_cast<InboundVariablesContext*>(Previous)->SetVariable(name, instance);
	}
}

ObjectInstance* InboundVariablesContext::TryFind(const std::wstring& name)
{
	if (auto search = Variables.find(name); search != Variables.end())
		return search->second;

	if (Previous == nullptr)
		return nullptr;

	return const_cast<InboundVariablesContext*>(Previous)->TryFind(name);
}

ObjectInstance* InboundVariablesContext::Find(const std::wstring& name)
{
	if (auto search = Variables.find(name); search != Variables.end())
		return search->second;

	if (Previous == nullptr)
		throw std::runtime_error("variable not found");

	return const_cast<InboundVariablesContext*>(Previous)->Find(name);
}

InboundVariablesContext::~InboundVariablesContext()
{
	for (auto const& instance : Variables)
		GarbageCollector::DestroyInstance(instance.second);

	Variables.clear();
}
