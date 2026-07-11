#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace shard
{
	class SHARD_API SemanticScope
	{
	public:
		std::unordered_map<std::wstring, SyntaxSymbol*> _symbols;
	
		SemanticScope* Parent;
		SyntaxSymbol* Owner;
		NamespaceNode* Namespace = nullptr;

		std::unordered_set<NamespaceNode*> ImportedNamespaces;
		std::unordered_set<std::wstring> AmbiguousNames;

		bool ReturnFound = false;
		bool ReturnsAnything = false;

		inline SemanticScope(SyntaxSymbol* owner, SemanticScope* parent)
			: Owner(owner), Parent(parent) { }

		inline ~SemanticScope()
		{

		}

		std::optional<SyntaxSymbol*> Lookup(const std::wstring& name);
		void DeclareSymbol(SyntaxSymbol* symbol);
	};
}