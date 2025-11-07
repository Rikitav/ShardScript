#pragma once
#include <shard/syntax/SyntaxSymbol.h>

#include <string>
#include <unordered_map>

namespace shard::parsing::semantic
{
	class SemanticScope
	{
		std::unordered_map<std::wstring, shard::syntax::SyntaxSymbol*> _symbols;
	
	public:
		const SemanticScope* Parent;
		const shard::syntax::SyntaxSymbol* Owner;

		bool ReturnFound = false;
		bool ReturnsAnything = false;

		inline SemanticScope(const shard::syntax::SyntaxSymbol* owner, SemanticScope* parent = nullptr)
			: Owner(owner), Parent(parent) { }

        shard::syntax::SyntaxSymbol* Lookup(std::wstring& name);
        void DeclareSymbol(shard::syntax::SyntaxSymbol* symbol);
		void RemoveSymbol(shard::syntax::SyntaxSymbol* symbol);
	};
}