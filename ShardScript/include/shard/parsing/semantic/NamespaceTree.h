#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace shard
{
	class NamespaceSymbol;
}

namespace shard
{
	class SHARD_API NamespaceNode
	{
	public:
		std::vector<shard::NamespaceSymbol*> Owners;
		std::vector<shard::TypeSymbol*> Types;
		std::unordered_map<std::wstring, NamespaceNode*> Nodes;

		inline NamespaceNode()
		{

		}

		inline ~NamespaceNode()
		{
			for (const auto& node : Nodes)
				delete node.second;
		}

		NamespaceNode* Lookup(std::wstring name);
		NamespaceNode* LookupOrCreate(std::wstring name, shard::NamespaceSymbol* current);
	};

	class SHARD_API NamespaceTree
	{
	public:
		NamespaceNode* Root;

		inline NamespaceTree()
			: Root(new NamespaceNode()) { }

		inline ~NamespaceTree()
		{
			delete Root;
		}
	};
}