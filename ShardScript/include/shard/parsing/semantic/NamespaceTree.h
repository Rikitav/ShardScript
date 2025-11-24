#pragma once
#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace shard::parsing::semantic
{
	class NamespaceNode
	{
	public:
		std::vector<shard::syntax::symbols::NamespaceSymbol*> Owners;
		std::vector<shard::syntax::symbols::TypeSymbol*> Types;
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
		NamespaceNode* LookupOrCreate(std::wstring name, shard::syntax::symbols::NamespaceSymbol* current);
	};

	class NamespaceTree
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