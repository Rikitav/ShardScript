#include <shard/syntax/SymbolBuilder.hpp>

#include <shard/syntax/symbols/StructSymbol.hpp>

using namespace shard;

// =========================================================================
// NamespaceSymbol
// =========================================================================

SymbolBuilder<NamespaceSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    NamespaceSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Namespace(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Parent = parent;
    Symbol->FullName = parent
        ? parent->FullName + L"." + name
        : name;

    NamespaceNode* parentNode = parent
        ? parent->Node
        : ctx.GetSemanticModel().Namespaces->Root;

    Symbol->Node = parentNode->LookupOrCreate(name, Symbol);
}

SymbolBuilder<MethodSymbol> SymbolBuilder<NamespaceSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Context, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<ClassSymbol> SymbolBuilder<NamespaceSymbol>::AddClass(
    const std::wstring& name,
    SymbolAccesibility access)
{
    SymbolBuilder<ClassSymbol> builder(Context, name, Symbol);
    builder.Get()->Accesibility = access;
    return builder;
}

SymbolBuilder<NamespaceSymbol> SymbolBuilder<NamespaceSymbol>::AddNamespace(
    const std::wstring& name)
{
    SymbolBuilder<NamespaceSymbol> builder(Context, name, Symbol);
    return builder;
}

// =========================================================================
// TypeParameterSymbol
// =========================================================================

SymbolBuilder<TypeParameterSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.TypeParameter(name);
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

// =========================================================================
// ClassSymbol
// =========================================================================

SymbolBuilder<ClassSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Class(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->IsReferenceType = true;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<ConstructorSymbol> SymbolBuilder<ClassSymbol>::AddInit(
    SymbolAccesibility access)
{
    SymbolBuilder<ConstructorSymbol> builder(Context, access, Symbol);
    return builder;
}

SymbolBuilder<MethodSymbol> SymbolBuilder<ClassSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Context, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<ClassSymbol>::AddField(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<FieldSymbol> builder(Context, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<PropertySymbol> SymbolBuilder<ClassSymbol>::AddProperty(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<PropertySymbol> builder(Context, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<ClassSymbol>::AddIndexer(
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<IndexatorSymbol> builder(Context, L"indexer", type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<ClassSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Context, name, Symbol);
    return builder;
}

// =========================================================================
// StructSymbol
// =========================================================================

SymbolBuilder<StructSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Struct(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->IsReferenceType = true;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<ConstructorSymbol> SymbolBuilder<StructSymbol>::AddInit(
    SymbolAccesibility access)
{
    SymbolBuilder<ConstructorSymbol> builder(Context, access, Symbol);
    return builder;
}

SymbolBuilder<MethodSymbol> SymbolBuilder<StructSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Context, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<StructSymbol>::AddField(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<FieldSymbol> builder(Context, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<PropertySymbol> SymbolBuilder<StructSymbol>::AddProperty(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<PropertySymbol> builder(Context, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<StructSymbol>::AddIndexer(
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<IndexatorSymbol> builder(Context, L"indexer", type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<StructSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Context, name, Symbol);
    return builder;
}

// =========================================================================
// MethodSymbol
// =========================================================================

SymbolBuilder<MethodSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Method(name, returnType, linking);
    Symbol->Accesibility = access;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::AddParameter(
    const std::wstring& name,
    TypeSymbol* type)
{
    ParameterSymbol* parameter = Factory.Parameter(name, type);
    parameter->Parent = Symbol;
    Symbol->Parameters.push_back(parameter);
    return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::SetCallback(MethodSymbolDelegate callback)
{
    Symbol->FunctionPointer = callback;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::SetCallback(
    ShardManagedMethodCallback callback,
    void* userData)
{
    Symbol->ManagedCallback = callback;
    Symbol->ManagedCallbackUserData = userData;
    Symbol->FunctionPointer = nullptr;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<MethodSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Context, name, Symbol);
    // TODO: add type parameters reg
    return builder;
}

// =========================================================================
// AccessorSymbol
// =========================================================================

SymbolBuilder<AccessorSymbol>::SymbolBuilder(
    CompilationContext& ctx,
    bool isGetter,
    SymbolAccesibility access,
    PropertySymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = isGetter ? Factory.Getter(parent) : Factory.Setter(parent);

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + Symbol->Name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = Symbol->Name;
    }
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::SetCallback(MethodSymbolDelegate callback)
{
    Symbol->FunctionPointer = callback;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::SetCallback(
    ShardManagedMethodCallback callback,
    void* userData)
{
    Symbol->ManagedCallback = callback;
    Symbol->ManagedCallbackUserData = userData;
    Symbol->FunctionPointer = nullptr;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}

// =========================================================================
// FieldSymbol
// =========================================================================

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Field(name, type, linking);
    Symbol->Accesibility = access;
    Symbol->ReturnType = type;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& ctx,
    PropertySymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.BackingField(parent);
    if (parent != nullptr)
    {
        parent->OnSymbolDeclared(Symbol);
    }
}

// =========================================================================
// PropertySymbol
// =========================================================================

SymbolBuilder<PropertySymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Property(name, type, linking);
    Symbol->Accesibility = access;
    Symbol->ReturnType = type;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<FieldSymbol> SymbolBuilder<PropertySymbol>::AddBackingField()
{
    SymbolBuilder<FieldSymbol> builder(Context, Symbol);
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::AddGetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Context, true, access, Symbol);
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::AddSetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Context, false, access, Symbol);
    return builder;
}

// =========================================================================
// IndexatorSymbol
// =========================================================================

SymbolBuilder<IndexatorSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Indexator(name, type);
    Symbol->Linking = linking;
    Symbol->Accesibility = access;
    Symbol->ReturnType = type;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<IndexatorSymbol>& SymbolBuilder<IndexatorSymbol>::AddParameter(
    const std::wstring& name,
    TypeSymbol* type)
{
    ParameterSymbol* parameter = Factory.Parameter(name, type);
    parameter->Parent = Symbol;
    Symbol->Parameters.push_back(parameter);

    if (Symbol->Getter != nullptr)
        Symbol->Getter->Parameters.push_back(parameter);

    if (Symbol->Setter != nullptr)
        Symbol->Setter->Parameters.push_back(parameter);

    return *this;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<IndexatorSymbol>::AddBackingField()
{
    SymbolBuilder<FieldSymbol> builder(Context, Symbol);
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::AddGetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Context, true, access, Symbol);
    builder.Get()->Parameters = Symbol->Parameters;
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::AddSetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Context, false, access, Symbol);
    builder.Get()->Parameters = Symbol->Parameters;
    return builder;
}

// =========================================================================
// ConstructorSymbol
// =========================================================================

SymbolBuilder<ConstructorSymbol>::SymbolBuilder(
    CompilationContext& ctx,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Constructor(parent, access);

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L".init";
        parent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = L"init";
    }
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::AddParameter(
    const std::wstring & name,
    TypeSymbol * type)
{
    ParameterSymbol* parameter = Factory.Parameter(name, type);
    parameter->Parent = Symbol;
    Symbol->Parameters.push_back(parameter);
    return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::SetCallback(MethodSymbolDelegate callback)
{
    Symbol->FunctionPointer = callback;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}

SymbolBuilder<ConstructorSymbol>& SymbolBuilder<ConstructorSymbol>::SetCallback(
    ShardManagedMethodCallback callback,
    void* userData)
{
    Symbol->ManagedCallback = callback;
    Symbol->ManagedCallbackUserData = userData;
    Symbol->FunctionPointer = nullptr;
    Symbol->HandleType = MethodHandleType::External;
    Symbol->IsExtern = true;
    return *this;
}
