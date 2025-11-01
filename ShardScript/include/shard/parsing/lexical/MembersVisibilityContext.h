#pragma once

/*
#include <shard/parsing/structures/ObjectInfo.h>

#include <unordered_map>
#include <string>

namespace shard::parsing
{
	class MembersVisibilityContext
	{
	public:
		MembersVisibilityContext* ParentContext;
		std::unordered_map<std::wstring, ObjectInfo*> Objects;

		inline MembersVisibilityContext(MembersVisibilityContext* parentContext)
			: ParentContext(parentContext) { }

		inline ~MembersVisibilityContext()
		{
			ParentContext = nullptr;
		}

		void AddObject(std::wstring name, const ObjectInfo* info);
		ObjectInfo* TryFind(std::wstring name);
	};
}
*/
