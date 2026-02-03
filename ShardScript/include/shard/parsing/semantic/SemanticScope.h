#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <string>
#include <unordered_map>

namespace shard
{
	class SHARD_API SemanticScope
	{
	public:
		std::unordered_map<std::wstring, shard::SyntaxSymbol*> _symbols;
	
		SemanticScope *const Parent;
		shard::SyntaxSymbol *const Owner;
		NamespaceNode* Namespace = nullptr;

		bool ReturnFound = false;
		bool ReturnsAnything = false;

		inline SemanticScope(shard::SyntaxSymbol *const owner, SemanticScope *const parent)
			: Owner(owner), Parent(parent) { }

		shard::SyntaxSymbol *const Lookup(const std::wstring& name);
		void DeclareSymbol(shard::SyntaxSymbol *const symbol);
		void RemoveSymbol(shard::SyntaxSymbol *const symbol);
	};
}