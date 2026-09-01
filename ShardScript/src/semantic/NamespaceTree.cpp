#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <string>

using namespace shard;

NamespaceNode* NamespaceNode::Lookup(std::wstring name)
{
	auto lookup = Nodes.find(name);
	return lookup == Nodes.end() ? nullptr : lookup->second;
}

NamespaceNode* NamespaceNode::Lookup(std::wstring_view name)
{
	for (const auto& node : Nodes)
	{
		if (node.first == name)
			return node.second;
	}

	return nullptr;
}

NamespaceNode* NamespaceNode::LookupOrCreate(std::wstring name, NamespaceSymbol* current)
{
	auto lookup = Nodes.find(name);
	if (lookup == Nodes.end())
	{
		NamespaceNode* newNode = new NamespaceNode();
		Nodes[name] = newNode;

		newNode->Owners.push_back(current);
		return newNode;
	}
	else
	{
		NamespaceNode* node = lookup->second;
		node->Owners.push_back(current);
		return node;
	}
}