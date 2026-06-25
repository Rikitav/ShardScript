#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/CompilationContext.hpp>
#include <shard/syntax/SymbolFactory.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/EnumSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>
#include <shard/syntax/symbols/OperatorSymbol.hpp>
#include <shard/syntax/TokenType.hpp>

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

        SymbolFactory& GetFactory() { return Factory; }
        T* Get() const { return Symbol; }
        operator T*() const { return Symbol; }
    };

    template<>
    class SHARD_API SymbolBuilder<NamespaceSymbol> : public SymbolBuilderBase<NamespaceSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, NamespaceSymbol* parent = nullptr);

        SymbolBuilder<MethodSymbol> AddMethod(
            const std::wstring& name,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<ClassSymbol> AddClass(
            const std::wstring& name,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<EnumSymbol> AddEnum(
            const std::wstring& name,
            bool isFlags = false,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<NamespaceSymbol> AddNamespace(
            const std::wstring& name);
    };

    template<>
    class SHARD_API SymbolBuilder<TypeParameterSymbol> : public SymbolBuilderBase<TypeParameterSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, SyntaxSymbol* parent);
    };

    template<>
    class SHARD_API SymbolBuilder<FieldSymbol> : public SymbolBuilderBase<FieldSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access,
            TypeSymbol* parent);

        SymbolBuilder(CompilationContext& ctx,
            PropertySymbol* parent);
    };

    template<>
    class SHARD_API SymbolBuilder<ConstructorSymbol> : public SymbolBuilderBase<ConstructorSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            SymbolAccesibility access,
            TypeSymbol* parent);

        SymbolBuilder<ConstructorSymbol>& AddParameter(
            const std::wstring& name,
            TypeSymbol* type);

        SymbolBuilder<ConstructorSymbol>& SetCallback(
            MethodSymbolDelegate callback);

        /*
        SymbolBuilder<ConstructorSymbol>& SetCallback(
            ShardManagedMethodCallback callback,
            void* userData = nullptr);
        */

        SymbolBuilder<TypeParameterSymbol> AddTypeParameter(
            const std::wstring& name);
    };

    template<>
    class SHARD_API SymbolBuilder<MethodSymbol> : public SymbolBuilderBase<MethodSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access,
            SyntaxSymbol* parent);

        SymbolBuilder<MethodSymbol>& AddParameter(
            const std::wstring& name,
            TypeSymbol* type);

        SymbolBuilder<MethodSymbol>& SetCallback(
            MethodSymbolDelegate callback);

        /*
        SymbolBuilder<MethodSymbol>& SetCallback(
            ShardManagedMethodCallback callback,
            void* userData = nullptr);
        */

        SymbolBuilder<TypeParameterSymbol> AddTypeParameter(
            const std::wstring& name);
    };

    template<>
    class SHARD_API SymbolBuilder<AccessorSymbol> : public SymbolBuilderBase<AccessorSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            bool isGetter,
            SymbolAccesibility access,
            PropertySymbol* parent);

        SymbolBuilder<AccessorSymbol>& SetCallback(
            MethodSymbolDelegate callback);

        /*
        SymbolBuilder<AccessorSymbol>& SetCallback(
            ShardManagedMethodCallback callback,
            void* userData = nullptr);
        */
    };

    template<>
    class SHARD_API SymbolBuilder<PropertySymbol> : public SymbolBuilderBase<PropertySymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access,
            TypeSymbol* parent);

        SymbolBuilder<FieldSymbol> AddBackingField();

        SymbolBuilder<AccessorSymbol> AddGetter(
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<AccessorSymbol> AddSetter(
            SymbolAccesibility access = SymbolAccesibility::Public);
    };

    template<>
    class SHARD_API SymbolBuilder<IndexatorSymbol> : public SymbolBuilderBase<IndexatorSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access,
            TypeSymbol* parent);

        SymbolBuilder<FieldSymbol> AddBackingField();

        SymbolBuilder<IndexatorSymbol>& AddParameter(
            const std::wstring& name,
            TypeSymbol* type);

        SymbolBuilder<AccessorSymbol> AddGetter(
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<AccessorSymbol> AddSetter(
            SymbolAccesibility access = SymbolAccesibility::Public);
    };

    template<>
    class SHARD_API SymbolBuilder<ClassSymbol> : public SymbolBuilderBase<ClassSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, SyntaxSymbol* parent);

        SymbolBuilder<ConstructorSymbol> AddInit(
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<MethodSymbol> AddMethod(
            const std::wstring& name,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<FieldSymbol> AddField(
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<PropertySymbol> AddProperty(
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<IndexatorSymbol> AddIndexer(
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<TypeParameterSymbol> AddTypeParameter(
            const std::wstring& name);

        SymbolBuilder<OperatorSymbol> AddOperator(
            TokenType opToken,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<ClassSymbol> Implements(
            TypeSymbol* interface);
    };

    template<>
    class SHARD_API SymbolBuilder<StructSymbol> : public SymbolBuilderBase<StructSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx, const std::wstring& name, SyntaxSymbol* parent);

        SymbolBuilder<ConstructorSymbol> AddInit(
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<MethodSymbol> AddMethod(
            const std::wstring& name,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<FieldSymbol> AddField(
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<PropertySymbol> AddProperty(
            const std::wstring& name,
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<IndexatorSymbol> AddIndexer(
            TypeSymbol* type,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<TypeParameterSymbol> AddTypeParameter(
            const std::wstring& name);

        SymbolBuilder<OperatorSymbol> AddOperator(
            TokenType opToken,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access = SymbolAccesibility::Public);

        SymbolBuilder<ClassSymbol> Implements(
            TypeSymbol* interface);
    };

    template<>
    class SHARD_API SymbolBuilder<EnumSymbol> : public SymbolBuilderBase<EnumSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            const std::wstring& name,
            bool isFlags,
            SyntaxSymbol* parent);

        SymbolBuilder<EnumSymbol>& SetFlags(bool value = true);

        SymbolBuilder<EnumSymbol>& AddValue(
            const std::wstring& name,
            std::int64_t value);
    };

    template<>
    class SHARD_API SymbolBuilder<OperatorSymbol> : public SymbolBuilderBase<OperatorSymbol>
    {
    public:
        SymbolBuilder(CompilationContext& ctx,
            TokenType opToken,
            TypeSymbol* returnType,
            SymbolLinking linking,
            SymbolAccesibility access,
            TypeSymbol* parent);

        SymbolBuilder<OperatorSymbol>& AddParameter(
            const std::wstring& name,
            TypeSymbol* type);

        SymbolBuilder<OperatorSymbol>& SetCallback(
            MethodSymbolDelegate callback);
    };
}
