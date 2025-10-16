#pragma once
#include <shard/runtime/VariableRegister.h>

#include <memory>
#include <unordered_map>
#include <string>

namespace shard::runtime
{
	class InboundVariablesContext
	{
	public:
		std::shared_ptr<InboundVariablesContext> Previous;
		std::unordered_map<std::string, std::shared_ptr<shard::runtime::VariableRegister>> Heap;

		~InboundVariablesContext();

		std::shared_ptr<VariableRegister> TryFind(std::string name);
	};
}