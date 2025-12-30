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
	
		const SemanticScope* Parent;
		const shard::SyntaxSymbol* Owner;
		NamespaceNode* Namespace = nullptr;

		bool ReturnFound = false;
		bool ReturnsAnything = false;

		inline SemanticScope(const shard::SyntaxSymbol* owner, SemanticScope* parent = nullptr)
			: Owner(owner), Parent(parent) { }

		shard::SyntaxSymbol* Lookup(const std::wstring& name);
		void DeclareSymbol(shard::SyntaxSymbol* symbol);
		void RemoveSymbol(shard::SyntaxSymbol* symbol);
	};
}