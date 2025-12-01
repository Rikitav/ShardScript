#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/ObjectInstance.h>

#include <unordered_map>
#include <string>

namespace shard::runtime
{
	class SHARD_API InboundVariablesContext
	{
	public:
		const InboundVariablesContext* Previous;
		std::unordered_map<std::wstring, ObjectInstance*> Variables;

		inline InboundVariablesContext(const InboundVariablesContext* prevVarContext)
			: Previous(prevVarContext) { }

		~InboundVariablesContext();

		ObjectInstance* AddVariable(const std::wstring name, ObjectInstance* instance);
		ObjectInstance* SetVariable(const std::wstring name, ObjectInstance* instance);
		ObjectInstance* TryFind(const std::wstring& name);
		ObjectInstance* Find(const std::wstring& name);
	};
}