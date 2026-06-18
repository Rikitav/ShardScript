#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxNode.hpp>

#include <shard/syntax/symbols/MemberSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>

#include <shard/parsing/semantic/SemanticScope.hpp>

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

namespace shard
{
    class InterfaceSymbol;
    class ClassSymbol;
    class FieldSymbol;
    class MethodSymbol;

    class SHARD_API SymbolTable
    {
        inline static const std::wstring GlobalTypeName = L"__GLOBAL__";

        std::unordered_map<SyntaxNode*, SyntaxSymbol*> nodeToSymbolMap;
        std::unordered_map<SyntaxSymbol*, SyntaxNode*> symbolToNodeMap;

        std::vector<std::unique_ptr<NamespaceSymbol>> namespacesList;
        std::vector<std::unique_ptr<TypeSymbol>> typesList;
        std::vector<std::unique_ptr<MemberSymbol>> membersList;
        std::vector<std::unique_ptr<SyntaxSymbol>> triviasList;

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

        struct StandardTypes
        {
            inline static SHARD_API InterfaceSymbol* IThrowable = nullptr;
            inline static SHARD_API InterfaceSymbol* IEnumerable = nullptr;
            inline static SHARD_API InterfaceSymbol* IDisposable = nullptr;

            inline static SHARD_API ClassSymbol* Runtime = nullptr;
            inline static SHARD_API MethodSymbol* RuntimeCaptureStackTrace = nullptr;

            inline static SHARD_API ClassSymbol* RuntimeException = nullptr;
            inline static SHARD_API FieldSymbol* RuntimeExceptionMessageField = nullptr;
            inline static SHARD_API FieldSymbol* RuntimeExceptionStackTraceField = nullptr;
        };

        SymbolTable();
        ~SymbolTable();

        SymbolTable(const SymbolTable&) = delete;
        SymbolTable& operator=(const SymbolTable&) = delete;

        void ClearSymbols();

        SyntaxSymbol* BindSymbol(SyntaxNode *const node, std::unique_ptr<SyntaxSymbol> symbol);
        SyntaxSymbol* ImplicitSymbol(std::unique_ptr<SyntaxSymbol> symbol);

        std::optional<SyntaxSymbol*> LookupSymbol(SyntaxNode *const node);
        std::optional<SyntaxNode*> LookupNode(SyntaxSymbol *const symbol);

        const std::vector<NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<TypeSymbol*> GetTypeSymbols();
        const std::vector<MethodSymbol*> GetMethodSymbols();
    };

    inline TypeSymbol*& TYPE_VOID = shard::SymbolTable::Primitives::Void;
    inline TypeSymbol*& TYPE_NULL = shard::SymbolTable::Primitives::Null;
    inline TypeSymbol*& TYPE_ANY = shard::SymbolTable::Primitives::Any;

    inline TypeSymbol*& TYPE_BOOL = shard::SymbolTable::Primitives::Boolean;
    inline TypeSymbol*& TYPE_INT = shard::SymbolTable::Primitives::Integer;
    inline TypeSymbol*& TYPE_DOUBLE = shard::SymbolTable::Primitives::Double;
    inline TypeSymbol*& TYPE_CHAR = shard::SymbolTable::Primitives::Char;
    inline TypeSymbol*& TYPE_STRING = shard::SymbolTable::Primitives::String;
    inline TypeSymbol*& TYPE_ARRAY = shard::SymbolTable::Primitives::Array;
}
