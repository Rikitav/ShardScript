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
    class NamespaceSymbol;
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
            static SHARD_API NamespaceSymbol* Namespace;
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
            inline static SHARD_API TypeSymbol* Byte;
        };

        struct StandardTypes
        {
            inline static SHARD_API InterfaceSymbol* IAsyncState = nullptr;
            inline static SHARD_API MethodSymbol* IAsyncState_MoveNext = nullptr;

            inline static SHARD_API InterfaceSymbol* IAwaitable = nullptr;
            inline static SHARD_API MethodSymbol* IAwaitable_GetAwaiter = nullptr;

            inline static SHARD_API InterfaceSymbol* IAwaiter = nullptr;
            inline static SHARD_API MethodSymbol* IAwaiter_IsCompleted = nullptr;
            inline static SHARD_API MethodSymbol* IAwaiter_OnCompleted = nullptr;
            inline static SHARD_API MethodSymbol* IAwaiter_GetResult = nullptr;

            inline static SHARD_API ClassSymbol* Task = nullptr;
            inline static SHARD_API FieldSymbol* Task_StateField = nullptr;
            inline static SHARD_API FieldSymbol* Task_ContinuationField = nullptr;
            inline static SHARD_API FieldSymbol* Task_ExceptionField = nullptr;
            inline static SHARD_API MethodSymbol* Task_MoveNext = nullptr;
            inline static SHARD_API MethodSymbol* Task_IsCompleted = nullptr;
            inline static SHARD_API MethodSymbol* Task_GetAwaiter = nullptr;
            inline static SHARD_API MethodSymbol* Task_GetResult = nullptr;
            inline static SHARD_API MethodSymbol* Task_OnCompleted = nullptr;
            inline static SHARD_API MethodSymbol* Task_Complete = nullptr;
            inline static SHARD_API MethodSymbol* Task_SetException = nullptr;
            inline static SHARD_API MethodSymbol* Task_InternalRoot = nullptr;
            inline static SHARD_API MethodSymbol* Task_Delay = nullptr;
            inline static SHARD_API MethodSymbol* Task_Shoot = nullptr;
            inline static SHARD_API MethodSymbol* Task_ShootGeneric = nullptr;
            inline static SHARD_API MethodSymbol* Task_Wait = nullptr;

            inline static SHARD_API ClassSymbol* ValueTask = nullptr;
            inline static SHARD_API TypeParameterSymbol* ValueTask_T = nullptr;
            inline static SHARD_API FieldSymbol* ValueTask_StateField = nullptr;
            inline static SHARD_API FieldSymbol* ValueTask_ResultField = nullptr;
            inline static SHARD_API FieldSymbol* ValueTask_ContinuationField = nullptr;
            inline static SHARD_API FieldSymbol* ValueTask_ExceptionField = nullptr;
            inline static SHARD_API AccessorSymbol* ValueTask_IsCompleted_get = nullptr;
            inline static SHARD_API AccessorSymbol* ValueTask_Result_get = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_MoveNext = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_GetAwaiter = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_GetResult = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_OnCompleted = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_InternalRoot = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_FromResult = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_SetResult = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_SetException = nullptr;
            inline static SHARD_API MethodSymbol* ValueTask_Shoot = nullptr;
            
            inline static SHARD_API MethodSymbol* Wait_Task = nullptr;
            inline static SHARD_API MethodSymbol* Wait_ValueTask = nullptr;

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

            inline static SHARD_API ClassSymbol* NativeContinuation = nullptr;
            inline static SHARD_API MethodSymbol* NativeContinuation_MoveNext = nullptr;
        };

        SymbolTable();
        ~SymbolTable();

        SymbolTable(const SymbolTable&) = delete;
        SymbolTable& operator=(const SymbolTable&) = delete;

        void ClearSymbols();

        SyntaxSymbol* BindSymbol(SyntaxNode* node, std::unique_ptr<SyntaxSymbol> symbol);
        SyntaxSymbol* ImplicitSymbol(std::unique_ptr<SyntaxSymbol> symbol);

        template<typename T>
        inline T* BindSymbolEx(SyntaxNode* node, std::unique_ptr<T> symbol)
        {
            return static_cast<T*>(BindSymbol(node, std::move(symbol)));
        }

        template<typename T>
        inline T* ImplicitSymbolEx(std::unique_ptr<T> symbol)
        {
            return static_cast<T*>(ImplicitSymbol(std::move(symbol)));
        }

        std::optional<SyntaxSymbol*> LookupSymbol(SyntaxNode* node);
        std::optional<SyntaxNode*> LookupNode(SyntaxSymbol* symbol);

        const std::vector<NamespaceSymbol*> GetNamespaceSymbols();
        const std::vector<TypeSymbol*> GetTypeSymbols();
        const std::vector<MethodSymbol*> GetMethodSymbols();

        void MarkAllSymbolsReady();
        void MarkJustCreatedSymbolsReady();

    private:
        static void ResolveGlobalComponents(SymbolTable* globalTable);

        static void ResolvePrimitives(SymbolTable* globalTable);
        static void ResolvePrimitiveOperators(SymbolTable* globalTable);
        static void ResolvePrimitivePrintables(SymbolTable* globalTable);

        static void ResolveGlobalMethods(SymbolTable* globalTable);
        static void ResolveInterfaces(SymbolTable* globalTable);
        static void ResolveEnumerables(SymbolTable* globalTable);
        static void ResolveExceptions(SymbolTable* globalTable);
        static void ResolveAsyncTypes(SymbolTable* globalTable);
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
    inline TypeSymbol*& TYPE_BYTE = shard::SymbolTable::Primitives::Byte;

    inline InterfaceSymbol*& TRAIT_ASYNCSTATE = shard::SymbolTable::StandardTypes::IAsyncState;
    inline MethodSymbol*& TRAIT_ASYNCSTATE_MoveNext = shard::SymbolTable::StandardTypes::IAsyncState_MoveNext;
    
    inline InterfaceSymbol*& TRAIT_AWAITABLE = shard::SymbolTable::StandardTypes::IAwaitable;
    inline MethodSymbol*& TRAIT_AWAITABLE_GetAwaiter = shard::SymbolTable::StandardTypes::IAwaitable_GetAwaiter;
    
    inline InterfaceSymbol*& TRAIT_AWAITER = shard::SymbolTable::StandardTypes::IAwaiter;
    inline MethodSymbol*& TRAIT_AWAITER_IsCompleted = shard::SymbolTable::StandardTypes::IAwaiter_IsCompleted;
    inline MethodSymbol*& TRAIT_AWAITER_OnCompleted = shard::SymbolTable::StandardTypes::IAwaiter_OnCompleted;
    inline MethodSymbol*& TRAIT_AWAITER_GetResult = shard::SymbolTable::StandardTypes::IAwaiter_GetResult;
    
    inline ClassSymbol*& CLASS_TASK = shard::SymbolTable::StandardTypes::Task;
    inline FieldSymbol*& CLASS_TASK_StateField = shard::SymbolTable::StandardTypes::Task_StateField;
    inline FieldSymbol*& CLASS_TASK_ContinuationField = shard::SymbolTable::StandardTypes::Task_ContinuationField;
    inline FieldSymbol*& CLASS_TASK_ExceptionField = shard::SymbolTable::StandardTypes::Task_ExceptionField;
    inline MethodSymbol*& CLASS_TASK_Shoot = shard::SymbolTable::StandardTypes::Task_Shoot;

    inline ClassSymbol*& CLASS_VALUETASK = shard::SymbolTable::StandardTypes::ValueTask;
    inline TypeParameterSymbol*& CLASS_VALUETASK_T = shard::SymbolTable::StandardTypes::ValueTask_T;
    inline FieldSymbol*& CLASS_VALUETASK_StateField = shard::SymbolTable::StandardTypes::ValueTask_StateField;
    inline FieldSymbol*& CLASS_VALUETASK_ResultField = shard::SymbolTable::StandardTypes::ValueTask_ResultField;
    inline FieldSymbol*& CLASS_VALUETASK_ContinuationField = shard::SymbolTable::StandardTypes::ValueTask_ContinuationField;
    inline FieldSymbol*& CLASS_VALUETASK_ExceptionField = shard::SymbolTable::StandardTypes::ValueTask_ExceptionField;
    inline MethodSymbol*& CLASS_VALUETASK_Shoot = shard::SymbolTable::StandardTypes::ValueTask_Shoot;

    inline InterfaceSymbol*& TRAIT_DISPOSABLE = shard::SymbolTable::StandardTypes::IDisposable;
    inline MethodSymbol*& TRAIT_DISPOSABLE_Dispose = shard::SymbolTable::StandardTypes::IDisposable_Dispose;

    inline InterfaceSymbol*& TRAIT_PRINTABLE = shard::SymbolTable::StandardTypes::IPrintable;
    inline MethodSymbol*& TRAIT_PRINTABLE_ToString = shard::SymbolTable::StandardTypes::IPrintable_ToString;

    inline InterfaceSymbol*& TRAIT_THROWABLE = shard::SymbolTable::StandardTypes::IThrowable;
    inline AccessorSymbol*& TRAIT_THROWABLE_getStackTrace = shard::SymbolTable::StandardTypes::IThrowable_getStackTrace;
    inline AccessorSymbol*& TRAIT_THROWABLE_getMessage = shard::SymbolTable::StandardTypes::IThrowable_getMessage;

    inline InterfaceSymbol*& TRAIT_ENUMERABLE = shard::SymbolTable::StandardTypes::IEnumerable;
    inline MethodSymbol*& TRAIT_ENUMERABLE_GETENUMERATOR = shard::SymbolTable::StandardTypes::IEnumerable_GetEnumerator;

    inline InterfaceSymbol*& TRAIT_ENUMERATOR = shard::SymbolTable::StandardTypes::IEnumerator;
    inline AccessorSymbol*& TRAIT_ENUMERATOR_CURRENT_GET = shard::SymbolTable::StandardTypes::IEnumerator_Current_get;
    inline MethodSymbol*& TRAIT_ENUMERATOR_MOVENEXT = shard::SymbolTable::StandardTypes::IEnumerator_MoveNext;

    inline ClassSymbol*& CLASS_ARRAYENUMERATOR = shard::SymbolTable::StandardTypes::ArrayEnumerator;
    inline TypeParameterSymbol*& CLASS_ARRAYENUMERATOR_T = shard::SymbolTable::StandardTypes::ArrayEnumerator_T;
    inline FieldSymbol*& CLASS_ARRAYENUMERATOR_SourceField = shard::SymbolTable::StandardTypes::ArrayEnumerator_SourceField;
    inline FieldSymbol*& CLASS_ARRAYENUMERATOR_IndexField = shard::SymbolTable::StandardTypes::ArrayEnumerator_IndexField;
    inline FieldSymbol*& CLASS_ARRAYENUMERATOR_LengthField = shard::SymbolTable::StandardTypes::ArrayEnumerator_LengthField;
}
