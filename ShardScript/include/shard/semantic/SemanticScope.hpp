#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <string>
#include <unordered_map>
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