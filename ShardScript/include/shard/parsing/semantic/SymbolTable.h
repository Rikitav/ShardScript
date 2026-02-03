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
            inline static SHARD_API shard::TypeSymbol *const Type = new shard::TypeSymbol(GlobalTypeName, shard::SyntaxKind::CompilationUnit);
            inline static SHARD_API shard::SemanticScope *const Scope = new SemanticScope(Type, nullptr);
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
        void BindSymbol(shard::SyntaxNode *const node, shard::SyntaxSymbol *const symbol);
        shard::SyntaxSymbol *const LookupSymbol(shard::SyntaxNode *const node);
        shard::SyntaxNode *const GetSyntaxNode(shard::SyntaxSymbol *const symbol);
        const std::vector<shard::NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<shard::TypeSymbol*> GetTypeSymbols();
    };
}
