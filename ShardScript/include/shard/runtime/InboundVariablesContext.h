#pragma once
#include <shard/runtime/ObjectInstance.h>
#include <unordered_map>
#include <string>

namespace shard::runtime
{
	class InboundVariablesContext
	{
	public:
		const InboundVariablesContext* Previous;
		std::unordered_map<std::wstring, ObjectInstance*> Variables;

		inline InboundVariablesContext(const InboundVariablesContext* prevVarContext)
			: Previous(prevVarContext) { }

		inline ~InboundVariablesContext();

		ObjectInstance* AddVariable(const std::wstring name, ObjectInstance* instance);
		ObjectInstance* TryFind(const std::wstring& name);
	};
}