#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>

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
		std::vector<NamespaceSymbol*> Owners;
		std::vector<SyntaxSymbol*> Members;
		std::unordered_map<std::wstring, NamespaceNode*> Nodes;

		inline NamespaceNode()
		{

		}

		inline ~NamespaceNode()
		{
			for (const auto& node : Nodes)
				delete node.second;
		}

		NamespaceNode(const NamespaceNode&) = delete;
		NamespaceNode& operator=(const NamespaceNode&) = delete;

		NamespaceNode* Lookup(std::wstring name);
		NamespaceNode* LookupOrCreate(std::wstring name, NamespaceSymbol* current);
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

		NamespaceTree(const NamespaceTree&) = delete;
		NamespaceTree& operator=(const NamespaceTree&) = delete;
	};
}