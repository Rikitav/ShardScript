#include <shard/parsing/semantic/NamespaceTree.h>
#include <string>
#include <shard/syntax/symbols/NamespaceSymbol.h>

using namespace shard::parsing::semantic;
using namespace shard::syntax::symbols;

NamespaceNode* NamespaceNode::Lookup(std::wstring name)
{
	auto lookup = Nodes.find(name);
	return lookup == Nodes.end() ? nullptr : lookup->second;
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