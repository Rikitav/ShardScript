/*
#include <shard/parsing/structures/MembersVisibilityContext.h>
#include <shard/parsing/structures/ObjectInfo.h>
#include <string>

using namespace std;
using namespace shard::parsing;

void MembersVisibilityContext::AddObject(wstring name, const ObjectInfo* info)
{
	Objects[name] = (ObjectInfo*)info;
}

ObjectInfo* MembersVisibilityContext::TryFind(wstring name)
{
	auto find = Objects.find(name);
	if (find != Objects.end())
		return find->second;

	return ParentContext != nullptr ? ParentContext->TryFind(name) : nullptr;
}
*/