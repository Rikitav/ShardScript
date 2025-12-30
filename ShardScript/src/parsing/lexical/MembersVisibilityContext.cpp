/*
#include <shard/parsing/structures/MembersVisibilityContext.h>
#include <shard/parsing/structures/ObjectInfo.h>
#include <string>

using namespace shard;

void MembersVisibilityContext::AddObject(std::wstring name, const ObjectInfo* info)
{
	Objects[name] = (ObjectInfo*)info;
}

ObjectInfo* MembersVisibilityContext::TryFind(std::wstring name)
{
	auto find = Objects.find(name);
	if (find != Objects.end())
		return find->second;

	return ParentContext != nullptr ? ParentContext->TryFind(name) : nullptr;
}
*/