#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>

#include <shard/parsing/semantic/SemanticScope.hpp>

#include <unordered_map>
#include <vector>
#include <string>
#include <ranges>

namespace shard
{
    class SHARD_API SymbolTable
    {
        inline static const std::wstring GlobalTypeName = L"<>_GLOBAL";

        std::unordered_map<SyntaxNode*, SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<SyntaxSymbol*, SyntaxNode*> symbolToNodeMap;
        std::vector<NamespaceSymbol*> namespacesList;
        std::vector<TypeSymbol*> typesList;

    public:
        struct Global
        {
            static SHARD_API TypeSymbol *const Type;
            static SHARD_API SemanticScope *const Scope;
        };

        struct Primitives
        {
            inline static SHARD_API TypeSymbol* Void;
            inline static SHARD_API TypeSymbol* Null;
            inline static SHARD_API TypeSymbol* Any;

            inline static SHARD_API TypeSymbol* Boolean;
            inline static SHARD_API TypeSymbol* Integer;
            inline static SHARD_API TypeSymbol* Double;
            inline static SHARD_API TypeSymbol* Char;
            inline static SHARD_API TypeSymbol* String;
            inline static SHARD_API TypeSymbol* Array;
        };

        SymbolTable();
        ~SymbolTable();

        void ClearSymbols();

        void BindSymbol(SyntaxNode *const node, SyntaxSymbol *const symbol);
        SyntaxSymbol *const LookupSymbol(SyntaxNode *const node);
        SyntaxNode *const GetSyntaxNode(SyntaxSymbol *const symbol);

        const std::vector<NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<TypeSymbol*> GetTypeSymbols();
    };
}
