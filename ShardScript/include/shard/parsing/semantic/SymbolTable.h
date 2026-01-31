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

namespace shard
{
    class SHARD_API SymbolTable
    {
        inline static const std::wstring GlobalTypeName = L"__GLOBAL__";

        std::unordered_map<shard::SyntaxNode*, shard::SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<shard::SyntaxSymbol*, shard::SyntaxNode*> symbolToNodeMap;
        std::vector<shard::NamespaceSymbol*> namespacesList;
        std::vector<shard::TypeSymbol*> typesList;

    public:
        struct Global
        {
            inline static SHARD_API shard::SemanticScope* Scope = new SemanticScope(nullptr, nullptr);
            inline static SHARD_API shard::TypeSymbol* Type = new shard::TypeSymbol(GlobalTypeName, shard::SyntaxKind::CompilationUnit);
        };

        struct Primitives
        {
            inline static SHARD_API shard::TypeSymbol* Void;
            inline static SHARD_API shard::TypeSymbol* Null;
            inline static SHARD_API shard::TypeSymbol* Any;

            inline static SHARD_API shard::TypeSymbol* Boolean;
            inline static SHARD_API shard::TypeSymbol* Integer;
            inline static SHARD_API shard::TypeSymbol* Double;
            inline static SHARD_API shard::TypeSymbol* Char;
            inline static SHARD_API shard::TypeSymbol* String;
            inline static SHARD_API shard::TypeSymbol* Array;
        };

        inline SymbolTable()
        {

        }

        ~SymbolTable();

        void ClearSymbols();
        void BindSymbol(shard::SyntaxNode* node, shard::SyntaxSymbol* symbol);
        shard::SyntaxSymbol* LookupSymbol(shard::SyntaxNode* node);
        shard::SyntaxNode* GetSyntaxNode(shard::SyntaxSymbol* symbol);
        std::vector<shard::NamespaceSymbol*> GetNamespaceSymbols();
        std::vector<shard::TypeSymbol*> GetTypeSymbols();
    };
}
