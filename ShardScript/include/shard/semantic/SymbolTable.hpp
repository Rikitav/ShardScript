#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxNode.hpp>

#include <shard/semantic/symbols/MemberSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>

#include <shard/semantic/SemanticScope.hpp>

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
    class AccessorSymbol;
    class TypeParameterSymbol;

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
            static SHARD_API TypeSymbol* Type;
            static SHARD_API SemanticScope* Scope;
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
            inline static SHARD_API TypeSymbol* NativeInteger;
        };

        struct StandardTypes
        {
            inline static SHARD_API InterfaceSymbol* IThrowable = nullptr;
            inline static SHARD_API AccessorSymbol* IThrowable_getMessage = nullptr;
            inline static SHARD_API AccessorSymbol* IThrowable_getStackTrace = nullptr;

            inline static SHARD_API InterfaceSymbol* IDisposable = nullptr;
            inline static SHARD_API MethodSymbol* IDisposable_Dispose = nullptr;

            inline static SHARD_API InterfaceSymbol* IPrintable = nullptr;
            inline static SHARD_API MethodSymbol* IPrintable_ToString = nullptr;

            inline static SHARD_API InterfaceSymbol* IEnumerable = nullptr;
            inline static SHARD_API MethodSymbol* IEnumerable_GetEnumerator = nullptr;

            inline static SHARD_API InterfaceSymbol* IEnumerator = nullptr;
            inline static SHARD_API MethodSymbol* IEnumerator_MoveNext = nullptr;
            inline static SHARD_API AccessorSymbol* IEnumerator_Current_get = nullptr;

            inline static SHARD_API ClassSymbol* ArrayEnumerator = nullptr;
            inline static SHARD_API TypeParameterSymbol* ArrayEnumerator_T = nullptr;
            inline static SHARD_API FieldSymbol* ArrayEnumerator_SourceField = nullptr;
            inline static SHARD_API FieldSymbol* ArrayEnumerator_IndexField = nullptr;
            inline static SHARD_API FieldSymbol* ArrayEnumerator_LengthField = nullptr;

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

        SyntaxSymbol* BindSymbol(SyntaxNode* node, std::unique_ptr<SyntaxSymbol> symbol);
        SyntaxSymbol* ImplicitSymbol(std::unique_ptr<SyntaxSymbol> symbol);

        std::optional<SyntaxSymbol*> LookupSymbol(SyntaxNode* node);
        std::optional<SyntaxNode*> LookupNode(SyntaxSymbol* symbol);

        const std::vector<NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<TypeSymbol*> GetTypeSymbols();
        const std::vector<MethodSymbol*> GetMethodSymbols();
    };

    inline TypeSymbol*& TYPE_VOID = shard::SymbolTable::Primitives::Void;
    inline TypeSymbol*& TYPE_NULL = shard::SymbolTable::Primitives::Null;
    inline TypeSymbol*& TYPE_ANY = shard::SymbolTable::Primitives::Any;
    inline TypeSymbol*& TYPE_NINT = shard::SymbolTable::Primitives::NativeInteger;

    inline TypeSymbol*& TYPE_BOOL = shard::SymbolTable::Primitives::Boolean;
    inline TypeSymbol*& TYPE_INT = shard::SymbolTable::Primitives::Integer;
    inline TypeSymbol*& TYPE_DOUBLE = shard::SymbolTable::Primitives::Double;
    inline TypeSymbol*& TYPE_CHAR = shard::SymbolTable::Primitives::Char;
    inline TypeSymbol*& TYPE_STRING = shard::SymbolTable::Primitives::String;
    inline TypeSymbol*& TYPE_ARRAY = shard::SymbolTable::Primitives::Array;

    inline InterfaceSymbol*& TRAIT_DISPOSABLE = shard::SymbolTable::StandardTypes::IDisposable;
    inline InterfaceSymbol*& TRAIT_PRINTABLE = shard::SymbolTable::StandardTypes::IPrintable;
    inline InterfaceSymbol*& TRAIT_THROWABLE = shard::SymbolTable::StandardTypes::IThrowable;
    inline InterfaceSymbol*& TRAIT_ENUMERABLE = shard::SymbolTable::StandardTypes::IEnumerable;
    inline MethodSymbol*& TRAIT_ENUMERABLE_GETENUMERATOR = shard::SymbolTable::StandardTypes::IEnumerable_GetEnumerator;

    inline InterfaceSymbol*& TRAIT_ENUMERATOR = shard::SymbolTable::StandardTypes::IEnumerator;
    inline MethodSymbol*& TRAIT_ENUMERATOR_MOVENEXT = shard::SymbolTable::StandardTypes::IEnumerator_MoveNext;
    inline AccessorSymbol*& TRAIT_ENUMERATOR_CURRENT_GET = shard::SymbolTable::StandardTypes::IEnumerator_Current_get;

    inline MethodSymbol*& TRAIT_DISPOSABLE_Dispose = shard::SymbolTable::StandardTypes::IDisposable_Dispose;
    inline MethodSymbol*& TRAIT_PRINTABLE_ToString = shard::SymbolTable::StandardTypes::IPrintable_ToString;
    
    inline AccessorSymbol*& TRAIT_THROWABLE_getStackTrace = shard::SymbolTable::StandardTypes::IThrowable_getStackTrace;
    inline AccessorSymbol*& TRAIT_THROWABLE_getMessage = shard::SymbolTable::StandardTypes::IThrowable_getMessage;
}
