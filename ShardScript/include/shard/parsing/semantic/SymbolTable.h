#pragma once
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxNode.h>
#include <unordered_map>
#include <vector>

namespace shard::parsing::semantic
{
    class SymbolTable
    {
        std::unordered_map<shard::syntax::SyntaxNode*, shard::syntax::SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<shard::syntax::SyntaxSymbol*, shard::syntax::SyntaxNode*> symbolToNodeMap;
    
    public:
        shard::syntax::symbols::MethodSymbol* EntryPoint = nullptr;
        SemanticScope* GlobalScope = new SemanticScope(nullptr, nullptr);

        struct Primitives
        {
            inline static shard::syntax::SyntaxSymbol* Void;
            inline static shard::syntax::symbols::TypeSymbol* Boolean;
            inline static shard::syntax::symbols::TypeSymbol* Integer;
            inline static shard::syntax::symbols::TypeSymbol* Char;
            inline static shard::syntax::symbols::TypeSymbol* String;
        };

        inline SymbolTable() { }

        void ResolvePrmitives();
        void BindSymbol(shard::syntax::SyntaxNode* node, shard::syntax::SyntaxSymbol* symbol);
        shard::syntax::SyntaxSymbol* LookupSymbol(shard::syntax::SyntaxNode* node);
        shard::syntax::SyntaxNode* GetSyntaxNode(shard::syntax::SyntaxSymbol* symbol);
        std::vector<shard::syntax::symbols::TypeSymbol*> GetTypeSymbols();
    };
}
