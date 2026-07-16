#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/parsing/SyntaxFacts.hpp>

using namespace shard;

// =========================================================================
// NamespaceSymbol
// =========================================================================

SymbolBuilder<NamespaceSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    NamespaceSymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.Namespace(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Parent = parent;
    Symbol->FullName = parent
        ? parent->FullName + L"." + name
        : name;

    NamespaceNode* parentNode = parent == nullptr ? SymbolTable::Global::Namespace->Node : parent->Node;
    Symbol->Node = parentNode->LookupOrCreate(name, Symbol);
}

SymbolBuilder<NamespaceSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    NamespaceSymbol* parent)
    : SymbolBuilderBase(ctx.GetSemanticModel().Table.get())
{
    Symbol = Factory.Namespace(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Parent = parent;
    Symbol->FullName = parent
        ? parent->FullName + L"." + name
        : name;

    //parentNode = SymbolTable::Global::Namespace->Node;
    Symbol->Node = parent == nullptr
        ? ctx.GetSemanticModel().Namespaces->Root->LookupOrCreate(name, Symbol)
        : parent->Node->LookupOrCreate(name, Symbol);
}

SymbolBuilder<MethodSymbol> SymbolBuilder<NamespaceSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Table, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<ClassSymbol> SymbolBuilder<NamespaceSymbol>::AddClass(
    const std::wstring& name,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<ClassSymbol> builder(Table, name, Symbol);
    builder.Get()->Accesibility = access;
    builder.Get()->Linking = linking;
    return builder;
}

SymbolBuilder<StructSymbol> SymbolBuilder<NamespaceSymbol>::AddStruct(
    const std::wstring& name,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<StructSymbol> builder(Table, name, Symbol);
    builder.Get()->Accesibility = access;
    builder.Get()->Linking = linking;
    return builder;
}

SymbolBuilder<EnumSymbol> SymbolBuilder<NamespaceSymbol>::AddEnum(
    const std::wstring& name,
    bool isFlags,
    SymbolAccesibility access)
{
    SymbolBuilder<EnumSymbol> builder(Table, name, isFlags, Symbol);
    builder.Get()->Accesibility = access;
    return builder;
}

SymbolBuilder<InterfaceSymbol> SymbolBuilder<NamespaceSymbol>::AddInterface(
    const std::wstring& name,
    SymbolAccesibility access)
{
    SymbolBuilder<InterfaceSymbol> builder(Table, name, Symbol);
    builder.Get()->Accesibility = access;
    return builder;
}

SymbolBuilder<NamespaceSymbol> SymbolBuilder<NamespaceSymbol>::AddNamespace(
    const std::wstring& name)
{
    SymbolBuilder<NamespaceSymbol> builder(Table, name, Symbol);
    return builder;
}

// =========================================================================
// TypeParameterSymbol
// =========================================================================

SymbolBuilder<TypeParameterSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.TypeParameter(name, static_cast<MethodSymbol*>(nullptr));
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

SymbolBuilder<TypeParameterSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, parent)
{
}

// =========================================================================
// ClassSymbol
// =========================================================================

SymbolBuilder<ClassSymbol>::SymbolBuilder(
    CompilationContext& ctx,
    ClassSymbol* symbol)
    : SymbolBuilderBase(ctx)
{
    Symbol = symbol;
}

SymbolBuilder<ClassSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.Class(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Inlining = TypeInlining::ByReference;
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

SymbolBuilder<ClassSymbol>::SymbolBuilder(SymbolTable* table,
    ClassSymbol* symbol)
    : SymbolBuilderBase(table)
{
    Symbol = symbol;
}

SymbolBuilder<ClassSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, parent)
{
}

SymbolBuilder<ConstructorSymbol> SymbolBuilder<ClassSymbol>::AddInit(
    SymbolAccesibility access)
{
    SymbolBuilder<ConstructorSymbol> builder(Table, access, Symbol);
    return builder;
}

SymbolBuilder<MethodSymbol> SymbolBuilder<ClassSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Table, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<ClassSymbol>::AddField(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<FieldSymbol> builder(Table, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<PropertySymbol> SymbolBuilder<ClassSymbol>::AddProperty(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<PropertySymbol> builder(Table, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<ClassSymbol>::AddIndexer(
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<IndexatorSymbol> builder(Table, L"indexer", type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<ClassSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Table, name, Symbol);
    return builder;
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<ClassSymbol>::AddOperator(
    TokenType opToken,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<OperatorSymbol> builder(Table, opToken, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<ClassSymbol>::AddCastOperator(
    TypeSymbol* targetType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<OperatorSymbol> builder(Table, TokenType::AsOperator, targetType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<ClassSymbol>& SymbolBuilder<ClassSymbol>::Implements(
    InterfaceSymbol* interface)
{
    Symbol->Interfaces.push_back(interface);
    return *this;
}

SymbolBuilder<ClassSymbol>& SymbolBuilder<ClassSymbol>::Implements(
    GenericTypeSymbol* interface)
{
    Symbol->Interfaces.push_back(interface);
    return *this;
}

SymbolBuilder<ClassSymbol>& SymbolBuilder<ClassSymbol>::DeclareGlobal()
{
    SymbolTable::Global::Scope->DeclareSymbol(Symbol);
    return *this;
}

SymbolBuilder<ClassSymbol>& SymbolBuilder<ClassSymbol>::SetFullName(const std::wstring& fullName)
{
    Symbol->FullName = fullName;
    return *this;
}

// =========================================================================
// StructSymbol
// =========================================================================

SymbolBuilder<StructSymbol>::SymbolBuilder(CompilationContext& ctx,
    StructSymbol* symbol)
    : SymbolBuilderBase(ctx.GetSemanticModel().Table.get())
{
    Symbol = symbol;
}

SymbolBuilder<StructSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.Struct(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Inlining = TypeInlining::ByReference;
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

SymbolBuilder<StructSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, parent)
{
    Symbol = Factory.Struct(name);
    Symbol->Accesibility = SymbolAccesibility::Public;
    Symbol->Inlining = TypeInlining::ByReference;
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

SymbolBuilder<StructSymbol>::SymbolBuilder(SymbolTable* table,
    StructSymbol* symbol)
    : SymbolBuilderBase(table)
{
    Symbol = symbol;
}

SymbolBuilder<ConstructorSymbol> SymbolBuilder<StructSymbol>::AddInit(
    SymbolAccesibility access)
{
    SymbolBuilder<ConstructorSymbol> builder(Table, access, Symbol);
    return builder;
}

SymbolBuilder<MethodSymbol> SymbolBuilder<StructSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Table, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<FieldSymbol> SymbolBuilder<StructSymbol>::AddField(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<FieldSymbol> builder(Table, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<PropertySymbol> SymbolBuilder<StructSymbol>::AddProperty(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<PropertySymbol> builder(Table, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<StructSymbol>::AddIndexer(
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<IndexatorSymbol> builder(Table, L"indexer", type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<StructSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Table, name, Symbol);
    return builder;
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<StructSymbol>::AddOperator(
    TokenType opToken,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<OperatorSymbol> builder(Table, opToken, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<OperatorSymbol> SymbolBuilder<StructSymbol>::AddCastOperator(
    TypeSymbol* targetType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<OperatorSymbol> builder(Table, TokenType::AsOperator, targetType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::Implements(
    InterfaceSymbol* interface)
{
    Symbol->Interfaces.push_back(interface);
    return *this;
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::Implements(
    GenericTypeSymbol* interface)
{
    Symbol->Interfaces.push_back(interface);
    return *this;
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::DeclareGlobal()
{
    SymbolTable::Global::Scope->DeclareSymbol(Symbol);
    return *this;
}

SymbolBuilder<StructSymbol>& SymbolBuilder<StructSymbol>::SetFullName(const std::wstring& fullName)
{
    Symbol->FullName = fullName;
    return *this;
}

// =========================================================================
// EnumSymbol
// =========================================================================

SymbolBuilder<EnumSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    bool isFlags,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.Enum(name, isFlags);
    Symbol->Accesibility = SymbolAccesibility::Public;
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

SymbolBuilder<EnumSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    bool isFlags,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, isFlags, parent)
{
}

SymbolBuilder<EnumSymbol>& SymbolBuilder<EnumSymbol>::SetFlags(bool value)
{
    Symbol->IsFlags = value;
    return *this;
}

SymbolBuilder<EnumSymbol>& SymbolBuilder<EnumSymbol>::AddValue(
    const std::wstring& name,
    std::int64_t value)
{
    FieldSymbol* field = Factory.EnumField(name, Symbol, value);
    Symbol->OnSymbolDeclared(field);
    return *this;
}

// =========================================================================
// MethodSymbol
// =========================================================================

SymbolBuilder<MethodSymbol>::SymbolBuilder(SymbolTable* table,
    MethodSymbol* symbol)
    : SymbolBuilderBase(table)
{
    Symbol = symbol;
}

SymbolBuilder<MethodSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
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

SymbolBuilder<MethodSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, returnType, linking, access, parent)
{
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
    return *this;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::IsImplementationOf(MethodSymbol* abstractMethod)
{
    if (abstractMethod == nullptr)
        return *this;

    SyntaxSymbol* parent = Symbol->Parent;
    if (parent == nullptr || !parent->IsType())
        return *this;

    TypeSymbol* parentType = static_cast<TypeSymbol*>(parent);
    parentType->InterfaceMethodMap[abstractMethod] = Symbol;
    return *this;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<MethodSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Table, name, Symbol);

    TypeParameterSymbol* typeParam = builder.Get();
    if (typeParam != nullptr)
    {
        std::uint16_t index = static_cast<std::uint16_t>(Symbol->TypeParameters.size() - 1);
        if (Symbol->Parent != nullptr && Symbol->Parent->IsType())
            index += static_cast<std::uint16_t>(static_cast<TypeSymbol*>(Symbol->Parent)->TypeParameters.size());

        typeParam->TypeArgumentIndex = index;
    }

    return builder;
}

SymbolBuilder<MethodSymbol>& SymbolBuilder<MethodSymbol>::DeclareGlobal()
{
    SymbolTable::Global::Scope->DeclareSymbol(Symbol);
    return *this;
}

// =========================================================================
// OperatorSymbol
// =========================================================================

SymbolBuilder<OperatorSymbol>::SymbolBuilder(SymbolTable* table,
    TokenType opToken,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(table)
{
    std::wstring name = GetOperatorMethodName(opToken);
    Symbol = Factory.Operator(name, opToken, returnType, nullptr, {});
    Symbol->Accesibility = access;
    Symbol->Linking = linking;
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

SymbolBuilder<OperatorSymbol>::SymbolBuilder(CompilationContext& ctx,
    TokenType opToken,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), opToken, returnType, linking, access, parent)
{
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::AddParameter(
    const std::wstring& name,
    TypeSymbol* type)
{
    ParameterSymbol* parameter = Factory.Parameter(name, type);
    parameter->Parent = Symbol;
    Symbol->Parameters.push_back(parameter);
    return *this;
}

SymbolBuilder<OperatorSymbol>& SymbolBuilder<OperatorSymbol>::SetCallback(MethodSymbolDelegate callback)
{
    Symbol->FunctionPointer = callback;
    Symbol->HandleType = MethodHandleType::External;
    return *this;
}

// =========================================================================
// AccessorSymbol
// =========================================================================

SymbolBuilder<AccessorSymbol>::SymbolBuilder(
    SymbolTable* table,
    bool isGetter,
    SymbolAccesibility access,
    PropertySymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = isGetter ? Factory.Getter(parent) : Factory.Setter(parent);
    Symbol->Accesibility = access;
    Symbol->Parent = parent;

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

SymbolBuilder<AccessorSymbol>::SymbolBuilder(
    CompilationContext& ctx,
    bool isGetter,
    SymbolAccesibility access,
    PropertySymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), isGetter, access, parent)
{
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::SetCallback(MethodSymbolDelegate callback)
{
    Symbol->FunctionPointer = callback;
    Symbol->HandleType = MethodHandleType::External;
    return *this;
}

SymbolBuilder<AccessorSymbol>& SymbolBuilder<AccessorSymbol>::IsImplementationOf(MethodSymbol* abstractMethod)
{
    if (abstractMethod == nullptr)
        return *this;

    SyntaxSymbol* parent = Symbol->Parent->Parent;
    if (parent == nullptr || !parent->IsType())
        return *this;

    TypeSymbol* parentType = static_cast<TypeSymbol*>(parent);
    parentType->InterfaceMethodMap[abstractMethod] = Symbol;
    return *this;
}

// =========================================================================
// FieldSymbol
// =========================================================================

SymbolBuilder<FieldSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(table)
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
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, type, linking, access, parent)
{
}

SymbolBuilder<FieldSymbol>::SymbolBuilder(SymbolTable* table,
    PropertySymbol* parent)
    : SymbolBuilderBase(table)
{
    Symbol = Factory.BackingField(parent);
    if (parent != nullptr && parent->Parent != nullptr)
    {
        parent->Parent->OnSymbolDeclared(Symbol);
    }
}

SymbolBuilder<FieldSymbol>::SymbolBuilder(CompilationContext& ctx,
    PropertySymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), parent)
{
}

// =========================================================================
// PropertySymbol
// =========================================================================

SymbolBuilder<PropertySymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(table)
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

SymbolBuilder<PropertySymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, type, linking, access, parent)
{
}

SymbolBuilder<FieldSymbol> SymbolBuilder<PropertySymbol>::AddBackingField()
{
    SymbolBuilder<FieldSymbol> builder(Table, Symbol);
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::AddGetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Table, true, access, Symbol);
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<PropertySymbol>::AddSetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Table, false, access, Symbol);
    return builder;
}

// =========================================================================
// IndexatorSymbol
// =========================================================================

SymbolBuilder<IndexatorSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(table)
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

SymbolBuilder<IndexatorSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, type, linking, access, parent)
{
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
    SymbolBuilder<FieldSymbol> builder(Table, Symbol);
    builder.Get()->Parent = Symbol->Parent;
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::AddGetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Table, true, access, Symbol);
    builder.Get()->Parameters = Symbol->Parameters;
    return builder;
}

SymbolBuilder<AccessorSymbol> SymbolBuilder<IndexatorSymbol>::AddSetter(SymbolAccesibility access)
{
    SymbolBuilder<AccessorSymbol> builder(Table, false, access, Symbol);
    AccessorSymbol* setter = builder.Get();

    // The factory creates the setter with the implicit 'value' parameter already
    // appended. Preserve it after copying the indexer parameters.
    ParameterSymbol* valueParam = setter->Parameters.empty() ? nullptr : setter->Parameters.back();
    setter->Parameters = Symbol->Parameters;
    if (valueParam != nullptr)
        setter->Parameters.push_back(valueParam);

    return builder;
}

// =========================================================================
// ConstructorSymbol
// =========================================================================

SymbolBuilder<ConstructorSymbol>::SymbolBuilder(
    SymbolTable* table,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilderBase(table)
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

SymbolBuilder<ConstructorSymbol>::SymbolBuilder(
    CompilationContext& ctx,
    SymbolAccesibility access,
    TypeSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), access, parent)
{
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
    return *this;
}

// =========================================================================
// InterfaceSymbol
// =========================================================================

SymbolBuilder<InterfaceSymbol>::SymbolBuilder(SymbolTable* table,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilderBase(table)
{
    SyntaxSymbol* actualParent = parent != nullptr ? parent : SymbolTable::Global::Namespace;
    Symbol = Factory.Interface(name, SymbolAccesibility::Public, actualParent);
    Symbol->Inlining = TypeInlining::ByReference;

    if (actualParent != nullptr)
    {
        Symbol->FullName = actualParent->FullName + L"." + name;
        actualParent->OnSymbolDeclared(Symbol);
    }
    else
    {
        Symbol->FullName = name;
    }
}

SymbolBuilder<InterfaceSymbol>::SymbolBuilder(CompilationContext& ctx,
    const std::wstring& name,
    SyntaxSymbol* parent)
    : SymbolBuilder(ctx.GetSemanticModel().Table.get(), name, parent)
{
}

SymbolBuilder<MethodSymbol> SymbolBuilder<InterfaceSymbol>::AddMethod(
    const std::wstring& name,
    TypeSymbol* returnType,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<MethodSymbol> builder(Table, name, returnType, linking, access, Symbol);
    return builder;
}

SymbolBuilder<PropertySymbol> SymbolBuilder<InterfaceSymbol>::AddProperty(
    const std::wstring& name,
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<PropertySymbol> builder(Table, name, type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<IndexatorSymbol> SymbolBuilder<InterfaceSymbol>::AddIndexer(
    TypeSymbol* type,
    SymbolLinking linking,
    SymbolAccesibility access)
{
    SymbolBuilder<IndexatorSymbol> builder(Table, L"indexer", type, linking, access, Symbol);
    return builder;
}

SymbolBuilder<TypeParameterSymbol> SymbolBuilder<InterfaceSymbol>::AddTypeParameter(const std::wstring& name)
{
    SymbolBuilder<TypeParameterSymbol> builder(Table, name, Symbol);
    return builder;
}

SymbolBuilder<InterfaceSymbol>& SymbolBuilder<InterfaceSymbol>::DeclareGlobal()
{
    SymbolTable::Global::Scope->DeclareSymbol(Symbol);
    return *this;
}

SymbolBuilder<InterfaceSymbol>& SymbolBuilder<InterfaceSymbol>::SetFullName(const std::wstring& fullName)
{
    Symbol->FullName = fullName;
    return *this;
}
