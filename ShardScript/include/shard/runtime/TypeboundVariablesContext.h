#pragma once
#include <shard/runtime/VariableRegister.h>

#include <memory>
#include <unordered_map>
#include <string>

namespace shard::runtime
{
	class TypeboundVariablesContext
	{
	public:
		std::shared_ptr<TypeboundVariablesContext> Previous;
		std::unordered_map<std::string, std::shared_ptr<shard::runtime::VariableRegister>> Fields;

		std::shared_ptr<VariableRegister> TryFind(std::string name);
	};
}