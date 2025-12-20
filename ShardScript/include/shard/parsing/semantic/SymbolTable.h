#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>

#include <shard/parsing/semantic/SemanticScope.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <ranges>

namespace shard::parsing::semantic
{
    class SHARD_API SymbolTable
    {
        inline static const std::wstring GlobalTypeName = L"__GLOBAL__";

        std::unordered_map<shard::syntax::SyntaxNode*, shard::syntax::SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<shard::syntax::SyntaxSymbol*, shard::syntax::SyntaxNode*> symbolToNodeMap;
        std::vector<shard::syntax::symbols::NamespaceSymbol*> namespacesList;
        std::vector<shard::syntax::symbols::TypeSymbol*> typesList;

    public:
        std::vector<shard::syntax::symbols::MethodSymbol*> EntryPointCandidates;
        
        struct Global
        {
            inline static SHARD_API shard::parsing::semantic::SemanticScope* Scope = new SemanticScope(nullptr, nullptr);
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Type = new shard::syntax::symbols::TypeSymbol(GlobalTypeName, shard::syntax::SyntaxKind::CompilationUnit);
        };

        struct Primitives
        {
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Void;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Null;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Any;

            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Boolean;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Integer;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Double;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Char;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* String;
            inline static SHARD_API shard::syntax::symbols::TypeSymbol* Array;
        };

        inline SymbolTable()
        {

        }

        ~SymbolTable();

        void ClearSymbols();
        void BindSymbol(shard::syntax::SyntaxNode* node, shard::syntax::SyntaxSymbol* symbol);
        shard::syntax::SyntaxSymbol* LookupSymbol(shard::syntax::SyntaxNode* node);
        shard::syntax::SyntaxNode* GetSyntaxNode(shard::syntax::SyntaxSymbol* symbol);
        std::vector<shard::syntax::symbols::NamespaceSymbol*> GetNamespaceSymbols();
        std::vector<shard::syntax::symbols::TypeSymbol*> GetTypeSymbols();
    };
}
