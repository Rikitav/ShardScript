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

SymbolBuilder<ClassSymbol> SymbolBuilder<NamespaceSymbol>::AddClass(
    const std::wstring& name,
    SymbolAccesibility access)
{
    SymbolBuilder<ClassSymbol> builder(Context, name, Symbol);
    builder.Get()->Accesibility = access;
    return builder;
}

SymbolBuilder<NamespaceSymbol> SymbolBuilder<NamespaceSymbol>::AddNamespace(
    const std::wstring& name,
    SymbolAccesibility access)
{
    SymbolBuilder<NamespaceSymbol> builder(Context, name, Symbol);
    builder.Get()->Accesibility = access;
    return builder;
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

        if (parent->Kind == SyntaxKind::NamespaceDeclaration)
        {
            static_cast<NamespaceSymbol*>(parent)->OnSymbolDeclared(Symbol);
        }
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<MethodSymbol> SymbolBuilder<ClassSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    bool isStatic,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Context, name, returnType, isStatic, access, Symbol);
    return builder;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<ClassSymbol>::AddField(
    const std::wstring& name,
    TypeSymbol* type,
    bool isStatic,
    SymbolAccesibility access)
{
    SymbolBuilder<FieldSymbol> builder(Context, name, type, isStatic, access, Symbol);
    return builder;
}

// =========================================================================
// MethodSymbol
// =========================================================================

SymbolBuilder<MethodSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* returnType,
    bool isStatic,
    SymbolAccesibility access,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Method(name, returnType, isStatic);
    Symbol->Accesibility = access;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;

        if (parent->IsType())
        {
            static_cast<TypeSymbol*>(parent)->OnSymbolDeclared(Symbol);
        }
        else if (parent->Kind == SyntaxKind::NamespaceDeclaration)
        {
            static_cast<NamespaceSymbol*>(parent)->OnSymbolDeclared(Symbol);
        }
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

// =========================================================================
// FieldSymbol
// =========================================================================

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    bool isStatic,
    SymbolAccesibility access,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(ctx)
{
    Symbol = Factory.Field(name, type, isStatic);
    Symbol->Accesibility = access;
    Symbol->ReturnType = type;
    Symbol->Parent = parent;

    if (parent != nullptr)
    {
        Symbol->FullName = parent->FullName + L"." + name;

        if (parent->IsType())
        {
            static_cast<TypeSymbol*>(parent)->OnSymbolDeclared(Symbol);
        }
    }
    else
    {
        Symbol->FullName = name;
    }
}
