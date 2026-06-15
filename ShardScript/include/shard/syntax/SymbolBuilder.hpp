#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolFactory.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>

#include <string>
#include <type_traits>

namespace shard
{
    template<typename T>
    class SymbolBuilder;

    namespace detail
    {
        template<typename T>
        struct IsSyntaxSymbol : std::is_base_of<SyntaxSymbol, T> {};
    }

    template<typename T>
    class SymbolBuilderBase
    {
        static_assert(detail::IsSyntaxSymbol<T>::value,
            "SymbolBuilder<T> can only be used with SyntaxSymbol-derived types");

    protected:
        CompilationContext& Context;
        SymbolFactory Factory;
        T* Symbol;

        SymbolBuilderBase(CompilationContext& ctx)
            : Context(ctx), Factory(ctx.GetSemanticModel().Table.get()), Symbol(nullptr) {}

    public:
        SymbolBuilderBase(const SymbolBuilderBase&) = delete;
        SymbolBuilderBase& operator=(const SymbolBuilderBase&) = delete;

        SymbolBuilderBase(SymbolBuilderBase&&) = default;
        SymbolBuilderBase& operator=(SymbolBuilderBase&&) = default;

        T* Get() const { return Symbol; }
        operator T*() const { return Symbol; }
    };

    template<>
    class SHARD_API SymbolBuilder<NamespaceSymbol> : public SymbolBuilderBase<NamespaceSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, NamespaceSymbol* parent = nullptr);

        SymbolBuilder<ClassSymbol> AddClass(
            const std::wstring& name,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<NamespaceSymbol> AddNamespace(
            const std::wstring& name,
            SymbolAccesibility access = SymbolAccesibility::Public);
    };

    template<>
    class SHARD_API SymbolBuilder<ClassSymbol> : public SymbolBuilderBase<ClassSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, SyntaxSymbol* parent);

        SymbolBuilder<MethodSymbol> AddMethod(
            const std::wstring& name,
            TypeSymbol* returnType,
            bool isStatic = true,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<FieldSymbol> AddField(
            const std::wstring& name,
            TypeSymbol* type,
            bool isStatic = false,
            SymbolAccesibility access = SymbolAccesibility::Public);
    };

    template<>
    class SHARD_API SymbolBuilder<MethodSymbol> : public SymbolBuilderBase<MethodSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* returnType,
            bool isStatic,
            SymbolAccesibility access,
            SyntaxSymbol* parent);

        SymbolBuilder<MethodSymbol>& AddParameter(
            const std::wstring& name,
            TypeSymbol* type);

        SymbolBuilder<MethodSymbol>& SetCallback(MethodSymbolDelegate callback);
        SymbolBuilder<MethodSymbol>& SetCallback(ShardManagedMethodCallback callback, void* userData = nullptr);
    };

    template<>
    class SHARD_API SymbolBuilder<FieldSymbol> : public SymbolBuilderBase<FieldSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* type,
            bool isStatic,
            SymbolAccesibility access,
            SyntaxSymbol* parent);
    };
}
