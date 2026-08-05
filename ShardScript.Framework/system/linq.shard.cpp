#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>

#include <ShardScript.hpp>

using namespace shard;

namespace
{
    static inline std::string thinify(const wchar_t* wstr)
    {
        size_t length = wcslen(wstr) + 1;
        std::string narrow(length, '\0');
        size_t converted = 0;

#ifdef _WIN32
        wcstombs_s(&converted, narrow.data(), length, wstr, _TRUNCATE);
#else
        wcstombs(narrow.data(), wstr, length);
#endif

        return narrow;
    }

    static TypeSymbol* GetElementTypeOfEnumerable(ObjectInstance* enumerable)
    {
        TypeSymbol* type = const_cast<TypeSymbol*>(enumerable->getInfo());
        if (type == nullptr)
            return nullptr;

        if (type->Kind == SyntaxKind::ArrayType)
            return static_cast<ArrayTypeSymbol*>(type)->UnderlayingType;

        GenericTypeSymbol* genericType = nullptr;
        TypeSymbol* searchType = type;
        if (type->Kind == SyntaxKind::GenericType)
        {
            genericType = static_cast<GenericTypeSymbol*>(type);
            searchType = genericType->UnderlayingType;
        }

        for (TypeSymbol* iface : searchType->Interfaces)
        {
            if (iface->Kind != SyntaxKind::GenericType)
                continue;

            GenericTypeSymbol* genericIface = static_cast<GenericTypeSymbol*>(iface);
            if (genericIface->UnderlayingType != TRAIT_ENUMERABLE)
                continue;

            TypeSymbol* elementType = genericIface->SubstituteTypeParameters(TRAIT_ENUMERABLE->TypeParameters[0]);
            if (elementType != nullptr && elementType->Kind == SyntaxKind::TypeParameter && genericType != nullptr)
            {
                TypeSymbol* resolved = genericType->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(elementType));
                if (resolved != nullptr)
                    elementType = resolved;
            }

            return elementType;
        }

        return nullptr;
    }

    static ObjectInstance* GetEnumerator(const CallState& context, ObjectInstance* enumerable)
    {
        TypeSymbol* enumerableType = const_cast<TypeSymbol*>(enumerable->getInfo());
        MethodSymbol* getEnumerator = enumerableType->FindInterfaceImplementation(TRAIT_ENUMERABLE_GETENUMERATOR);
        if (getEnumerator == nullptr)
            throw std::runtime_error("Type does not implement IEnumerable<T>");

        TypeSymbol* elementType = GetElementTypeOfEnumerable(enumerable);
        if (elementType == nullptr)
            throw std::runtime_error("Could not determine element type of IEnumerable<T>");

        enumerable->IncrementReference();
        context.Runtimer.SetPendingTypeArguments({ elementType });
        ObjectInstance* enumerator = context.Runtimer.InvokeMethod(getEnumerator, &enumerable, 1);
        if (enumerator == nullptr)
            throw std::runtime_error("GetEnumerator returned null");

        return enumerator;
    }

    static bool MoveNext(const CallState& context, ObjectInstance* enumerator)
    {
        TypeSymbol* enumeratorType = const_cast<TypeSymbol*>(enumerator->getInfo());
        MethodSymbol* moveNext = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_MOVENEXT);
        if (moveNext == nullptr)
            throw std::runtime_error("Enumerator does not implement MoveNext");

        enumerator->IncrementReference();
        ObjectInstance* result = context.Runtimer.InvokeMethod(moveNext, &enumerator, 1);
        return result != nullptr && result->AsBoolean();
    }

    static ObjectInstance* Current(const CallState& context, ObjectInstance* enumerator)
    {
        TypeSymbol* enumeratorType = const_cast<TypeSymbol*>(enumerator->getInfo());
        MethodSymbol* currentGetter = enumeratorType->FindInterfaceImplementation(TRAIT_ENUMERATOR_CURRENT_GET);
        if (currentGetter == nullptr)
            throw std::runtime_error("Enumerator does not implement Current getter");

        enumerator->IncrementReference();
        return context.Runtimer.InvokeMethod(currentGetter, &enumerator, 1);
    }

    static ObjectInstance* InvokeDelegateOneArg(const CallState& context, ObjectInstance* delegateObj, ObjectInstance* arg, TypeSymbol* typeArg)
    {
        if (delegateObj == nullptr || delegateObj->DelegateTarget == nullptr)
            throw std::runtime_error("Invalid delegate");

        MethodSymbol* method = delegateObj->DelegateTarget;
        ObjectInstance* args[] = { arg };

        context.Runtimer.SetPendingTypeArguments({ typeArg });
        return context.Runtimer.InvokeMethod(method, args, 1);
    }

    static ObjectInstance* InvokeDelegateOneArg(const CallState& context, ObjectInstance* delegateObj, ObjectInstance* arg, TypeSymbol* typeArg1, TypeSymbol* typeArg2)
    {
        if (delegateObj == nullptr || delegateObj->DelegateTarget == nullptr)
            throw std::runtime_error("Invalid delegate");

        MethodSymbol* method = delegateObj->DelegateTarget;
        ObjectInstance* args[] = { arg };

        context.Runtimer.SetPendingTypeArguments({ typeArg1, typeArg2 });
        return context.Runtimer.InvokeMethod(method, args, 1);
    }

    static DelegateTypeSymbol* CreateGenericFuncDelegate(SymbolTable* table, SymbolFactory& factory, NamespaceSymbol* ns)
    {
        auto delegateSymbol = std::make_unique<DelegateTypeSymbol>(L"Func");
        DelegateTypeSymbol* delegate = delegateSymbol.get();
        table->ImplicitSymbol(std::move(delegateSymbol));

        delegate->Parent = ns;
        delegate->FullName = ns->FullName + L".Func";
        delegate->Accesibility = SymbolAccesibility::Public;
        ns->OnSymbolDeclared(delegate);

        TypeParameterSymbol* tParam = factory.TypeParameter(L"T", delegate);
        tParam->FullName = delegate->FullName + L".T";
        delegate->TypeParameters.push_back(tParam);

        TypeParameterSymbol* uParam = factory.TypeParameter(L"U", delegate);
        uParam->FullName = delegate->FullName + L".U";
        delegate->TypeParameters.push_back(uParam);

        ParameterSymbol* param = factory.Parameter(L"value", tParam);

        auto anonymousMethod = std::make_unique<MethodSymbol>(L"");
        anonymousMethod->HandleType = MethodHandleType::Lambda;
        anonymousMethod->Accesibility = SymbolAccesibility::Public;
        anonymousMethod->ReturnType = uParam;
        anonymousMethod->Linking = LINK_STATIC;
        anonymousMethod->Parameters.push_back(param);

        delegate->ReturnType = uParam;
        delegate->Parameters.push_back(param);
        delegate->AnonymousSymbol = anonymousMethod.get();

        table->ImplicitSymbol(std::move(anonymousMethod));
        return delegate;
    }
}

// ============================================================================
// Select<T, U>
// ============================================================================

static ClassSymbol* selectEnumerableClass_raw = nullptr;
static ClassSymbol* selectEnumeratorClass_raw = nullptr;
static FieldSymbol* selectEnumerable_sourceField = nullptr;
static FieldSymbol* selectEnumerable_selectorField = nullptr;
static FieldSymbol* selectEnumerator_enumeratorField = nullptr;
static FieldSymbol* selectEnumerator_selectorField = nullptr;
static FieldSymbol* selectEnumerator_currentField = nullptr;

static ObjectInstance* shard_selectenumerator_MoveNext(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* sourceEnumerator = self->GetField(selectEnumerator_enumeratorField->SlotIndex);
    ObjectInstance* selector = self->GetField(selectEnumerator_selectorField->SlotIndex);

    if (!MoveNext(context, sourceEnumerator))
        return context.Collector.FromValue(false);

    ObjectInstance* element = Current(context, sourceEnumerator);
    TypeSymbol* inputType = context.Frame->TypeArguments[0];
    TypeSymbol* outputType = context.Frame->TypeArguments[1];

    ObjectInstance* transformed = InvokeDelegateOneArg(context, selector, element, inputType, outputType);
    self->SetField(selectEnumerator_currentField->SlotIndex, transformed);

    return context.Collector.FromValue(true);
}

static ObjectInstance* shard_selectenumerator_Current_get(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    return self->GetField(selectEnumerator_currentField->SlotIndex);
}

static ObjectInstance* shard_selectenumerable_GetEnumerator(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* source = self->GetField(selectEnumerable_sourceField->SlotIndex);
    ObjectInstance* selector = self->GetField(selectEnumerable_selectorField->SlotIndex);

    TypeSymbol* inputType = context.Frame->TypeArguments[0];
    TypeSymbol* outputType = context.Frame->TypeArguments[1];

    ObjectInstance* enumerator = context.Collector.AllocateGeneric(selectEnumeratorClass_raw, { inputType, outputType });
    ObjectInstance* sourceEnumerator = GetEnumerator(context, source);

    enumerator->SetField(selectEnumerator_enumeratorField->SlotIndex, sourceEnumerator);
    enumerator->SetField(selectEnumerator_selectorField->SlotIndex, selector);

    return enumerator;
}

static ObjectInstance* shard_linq_Select(const CallState& context) noexcept(false)
{
    ObjectInstance* source = context.Args[0];
    ObjectInstance* selector = context.Args[1];

    TypeSymbol* inputType = context.Frame->TypeArguments[0];
    TypeSymbol* outputType = context.Frame->TypeArguments[1];

    ObjectInstance* enumerable = context.Collector.AllocateGeneric(selectEnumerableClass_raw, { inputType, outputType });
    enumerable->SetField(selectEnumerable_sourceField->SlotIndex, source);
    enumerable->SetField(selectEnumerable_selectorField->SlotIndex, selector);

    return enumerable;
}

// ============================================================================
// Where<T>
// ============================================================================

static ClassSymbol* whereEnumerableClass_raw = nullptr;
static ClassSymbol* whereEnumeratorClass_raw = nullptr;
static FieldSymbol* whereEnumerable_sourceField = nullptr;
static FieldSymbol* whereEnumerable_predicateField = nullptr;
static FieldSymbol* whereEnumerator_enumeratorField = nullptr;
static FieldSymbol* whereEnumerator_predicateField = nullptr;
static FieldSymbol* whereEnumerator_currentField = nullptr;

static ObjectInstance* shard_whereenumerator_MoveNext(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* sourceEnumerator = self->GetField(whereEnumerator_enumeratorField->SlotIndex);
    ObjectInstance* predicate = self->GetField(whereEnumerator_predicateField->SlotIndex);

    TypeSymbol* elementType = context.Frame->TypeArguments[0];

    while (MoveNext(context, sourceEnumerator))
    {
        ObjectInstance* element = Current(context, sourceEnumerator);
        element->IncrementReference();
        ObjectInstance* predicateResult = InvokeDelegateOneArg(context, predicate, element, elementType);

        if (predicateResult != nullptr && predicateResult->AsBoolean())
        {
            self->SetField(whereEnumerator_currentField->SlotIndex, element);
            return context.Collector.FromValue(true);
        }

        element->DecrementReference();
    }

    return context.Collector.FromValue(false);
}

static ObjectInstance* shard_whereenumerator_Current_get(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    return self->GetField(whereEnumerator_currentField->SlotIndex);
}

static ObjectInstance* shard_whereenumerable_GetEnumerator(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* source = self->GetField(whereEnumerable_sourceField->SlotIndex);
    ObjectInstance* predicate = self->GetField(whereEnumerable_predicateField->SlotIndex);

    TypeSymbol* elementType = context.Frame->TypeArguments[0];

    ObjectInstance* enumerator = context.Collector.AllocateGeneric(whereEnumeratorClass_raw, { elementType });
    ObjectInstance* sourceEnumerator = GetEnumerator(context, source);

    enumerator->SetField(whereEnumerator_enumeratorField->SlotIndex, sourceEnumerator);
    enumerator->SetField(whereEnumerator_predicateField->SlotIndex, predicate);

    return enumerator;
}

static ObjectInstance* shard_linq_Where(const CallState& context) noexcept(false)
{
    if (context.Args.size() < 2)
        throw std::runtime_error("Where expects 2 arguments, got " + std::to_string(context.Args.size()));

    ObjectInstance* source = context.Args[0];
    ObjectInstance* predicate = context.Args[1];

    TypeSymbol* elementType = context.Frame->TypeArguments[0];

    ObjectInstance* enumerable = context.Collector.AllocateGeneric(whereEnumerableClass_raw, { elementType });
    enumerable->SetField(whereEnumerable_sourceField->SlotIndex, source);
    enumerable->SetField(whereEnumerable_predicateField->SlotIndex, predicate);

    return enumerable;
}

// ============================================================================
// OrderBy<T, TKey>
// ============================================================================

static ClassSymbol* orderByEnumerableClass_raw = nullptr;
static ClassSymbol* orderByEnumeratorClass_raw = nullptr;
static FieldSymbol* orderByEnumerable_sortedField = nullptr;
static FieldSymbol* orderByEnumerator_sortedField = nullptr;
static FieldSymbol* orderByEnumerator_indexField = nullptr;

static ObjectInstance* shard_orderbyenumerator_MoveNext(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* sortedArray = self->GetField(orderByEnumerator_sortedField->SlotIndex);
    std::int64_t index = self->GetField(orderByEnumerator_indexField->SlotIndex)->AsInteger();

    std::size_t length = sortedArray->GetArrayLength();
    index++;
    self->SetField(orderByEnumerator_indexField->SlotIndex, context.Collector.FromValue(index));

    return context.Collector.FromValue(static_cast<std::size_t>(index) < length);
}

static ObjectInstance* shard_orderbyenumerator_Current_get(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* sortedArray = self->GetField(orderByEnumerator_sortedField->SlotIndex);
    std::int64_t index = self->GetField(orderByEnumerator_indexField->SlotIndex)->AsInteger();

    return sortedArray->GetElement(static_cast<std::size_t>(index));
}

static ObjectInstance* shard_orderbyenumerable_GetEnumerator(const CallState& context) noexcept(false)
{
    ObjectInstance* self = context.Args[0];
    ObjectInstance* sortedArray = self->GetField(orderByEnumerable_sortedField->SlotIndex);

    TypeSymbol* elementType = context.Frame->TypeArguments[0];

    ObjectInstance* enumerator = context.Collector.AllocateGeneric(orderByEnumeratorClass_raw, { elementType });
    enumerator->SetField(orderByEnumerator_sortedField->SlotIndex, sortedArray);
    enumerator->SetField(orderByEnumerator_indexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)));

    return enumerator;
}

static ObjectInstance* shard_linq_OrderBy(const CallState& context) noexcept(false)
{
    ObjectInstance* source = context.Args[0];
    ObjectInstance* keySelector = context.Args[1];

    TypeSymbol* elementType = context.Frame->TypeArguments[0];
    TypeSymbol* keyType = context.Frame->TypeArguments[1];

    std::vector<ObjectInstance*> elements;
    {
        ObjectInstance* sourceEnumerator = GetEnumerator(context, source);
        while (MoveNext(context, sourceEnumerator))
        {
            ObjectInstance* element = Current(context, sourceEnumerator);
            element->IncrementReference();
            elements.push_back(element);
        }
    }

    std::vector<ObjectInstance*> sorted = elements;
    std::sort(sorted.begin(), sorted.end(), [&](ObjectInstance* a, ObjectInstance* b)
    {
        ObjectInstance* keyA = InvokeDelegateOneArg(context, keySelector, a, elementType, keyType);
        ObjectInstance* keyB = InvokeDelegateOneArg(context, keySelector, b, elementType, keyType);

        ObjectInstance* lessResult = context.Runtimer.InvokeOperatorMethod(keyA, TokenType::LessOperator, keyB);
        if (lessResult != nullptr && lessResult->AsBoolean())
            return true;

        ObjectInstance* greaterResult = context.Runtimer.InvokeOperatorMethod(keyA, TokenType::GreaterOperator, keyB);
        if (greaterResult != nullptr && greaterResult->AsBoolean())
            return false;

        return false;
    });

    ObjectInstance* sortedArray = context.Collector.AllocateArray(elementType, sorted.size());
    for (std::size_t i = 0; i < sorted.size(); i++)
    {
        sortedArray->SetElement(i, sorted[i]);
        sorted[i]->DecrementReference();
    }

    ObjectInstance* enumerable = context.Collector.AllocateGeneric(orderByEnumerableClass_raw, { elementType, keyType });
    enumerable->SetField(orderByEnumerable_sortedField->SlotIndex, sortedArray);

    return enumerable;
}

// ============================================================================
// Library metadata and entry point
// ============================================================================

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.linq";
    lib.Description = L"LINQ-style extension methods for IEnumerable<T>";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> linqNamespace(context, L"linq");
    SymbolFactory factory(context.GetSemanticModel().Table.get());
    SymbolTable* table = context.GetSemanticModel().Table.get();

    DelegateTypeSymbol* funcDelegate = CreateGenericFuncDelegate(table, factory, linqNamespace.Get());

    // ------------------------------------------------------------------------
    // Select<T, U>
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> selectEnumerableClass = linqNamespace.AddClass(L"SelectEnumerable");
        TypeParameterSymbol* selectEnumerable_T = selectEnumerableClass.AddTypeParameter(L"T").Get();
        TypeParameterSymbol* selectEnumerable_U = selectEnumerableClass.AddTypeParameter(L"U").Get();

        selectEnumerableClass_raw = selectEnumerableClass.Get();

        selectEnumerableClass
            .Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", selectEnumerable_U } }));

        selectEnumerable_sourceField = selectEnumerableClass
            .AddField(L"_source", factory.GenericType(TRAIT_ENUMERABLE, { { L"T", selectEnumerable_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        selectEnumerable_selectorField = selectEnumerableClass
            .AddField(L"_selector", factory.GenericType(funcDelegate, { { L"T", selectEnumerable_T }, { L"U", selectEnumerable_U } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        selectEnumerableClass.AddMethod(L"GetEnumerator",
            factory.GenericType(TRAIT_ENUMERATOR, { { L"T", selectEnumerable_U } }), LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
            .SetCallback(&shard_selectenumerable_GetEnumerator);
    }

    {
        SymbolBuilder<ClassSymbol> selectEnumeratorClass = linqNamespace.AddClass(L"SelectEnumerator");
        TypeParameterSymbol* selectEnumerator_T = selectEnumeratorClass.AddTypeParameter(L"T").Get();
        TypeParameterSymbol* selectEnumerator_U = selectEnumeratorClass.AddTypeParameter(L"U").Get();

        selectEnumeratorClass_raw = selectEnumeratorClass.Get();

        selectEnumeratorClass
            .Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", selectEnumerator_U } }));

        selectEnumerator_enumeratorField = selectEnumeratorClass
            .AddField(L"_enumerator", factory.GenericType(TRAIT_ENUMERATOR, { { L"T", selectEnumerator_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        selectEnumerator_selectorField = selectEnumeratorClass
            .AddField(L"_selector", factory.GenericType(funcDelegate, { { L"T", selectEnumerator_T }, { L"U", selectEnumerator_U } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        selectEnumerator_currentField = selectEnumeratorClass
            .AddField(L"_current", selectEnumerator_U, LINK_INSTANCE, ACS_PRIVATE).Get();

        selectEnumeratorClass.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
            .SetCallback(&shard_selectenumerator_MoveNext);

        selectEnumeratorClass.AddProperty(L"Current", selectEnumerator_U, LINK_INSTANCE)
            .AddGetter()
            .IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET)
            .SetCallback(&shard_selectenumerator_Current_get);
    }

    SymbolBuilder<MethodSymbol> selectMethod = linqNamespace.AddMethod(L"Select", TYPE_ANY, LINK_STATIC, ACS_PUBLIC);
    TypeParameterSymbol* select_T = selectMethod.AddTypeParameter(L"T").Get();
    TypeParameterSymbol* select_U = selectMethod.AddTypeParameter(L"U").Get();
    selectMethod.Get()->ReturnType = factory.GenericType(TRAIT_ENUMERABLE, { { L"T", select_U } });
    selectMethod
        .AddParameter(L"source", factory.GenericType(TRAIT_ENUMERABLE, { { L"T", select_T } }))
        .AddParameter(L"selector", factory.GenericType(funcDelegate, { { L"T", select_T }, { L"U", select_U } }))
        .SetCallback(&shard_linq_Select);

    // ------------------------------------------------------------------------
    // Where<T>
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> whereEnumerableClass = linqNamespace.AddClass(L"WhereEnumerable");
        TypeParameterSymbol* whereEnumerable_T = whereEnumerableClass.AddTypeParameter(L"T").Get();

        whereEnumerableClass_raw = whereEnumerableClass.Get();

        whereEnumerableClass
            .Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", whereEnumerable_T } }));

        whereEnumerable_sourceField = whereEnumerableClass
            .AddField(L"_source", factory.GenericType(TRAIT_ENUMERABLE, { { L"T", whereEnumerable_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        whereEnumerable_predicateField = whereEnumerableClass
            .AddField(L"_predicate", factory.GenericType(funcDelegate, { { L"T", whereEnumerable_T }, { L"U", TYPE_BOOL } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        whereEnumerableClass.AddMethod(L"GetEnumerator",
            factory.GenericType(TRAIT_ENUMERATOR, { { L"T", whereEnumerable_T } }), LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
            .SetCallback(&shard_whereenumerable_GetEnumerator);
    }

    {
        SymbolBuilder<ClassSymbol> whereEnumeratorClass = linqNamespace.AddClass(L"WhereEnumerator");
        TypeParameterSymbol* whereEnumerator_T = whereEnumeratorClass.AddTypeParameter(L"T").Get();

        whereEnumeratorClass_raw = whereEnumeratorClass.Get();

        whereEnumeratorClass
            .Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", whereEnumerator_T } }));

        whereEnumerator_enumeratorField = whereEnumeratorClass
            .AddField(L"_enumerator", factory.GenericType(TRAIT_ENUMERATOR, { { L"T", whereEnumerator_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        whereEnumerator_predicateField = whereEnumeratorClass
            .AddField(L"_predicate", factory.GenericType(funcDelegate, { { L"T", whereEnumerator_T }, { L"U", TYPE_BOOL } }), LINK_INSTANCE, ACS_PRIVATE).Get();

        whereEnumerator_currentField = whereEnumeratorClass
            .AddField(L"_current", whereEnumerator_T, LINK_INSTANCE, ACS_PRIVATE).Get();

        whereEnumeratorClass.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
            .SetCallback(&shard_whereenumerator_MoveNext);

        whereEnumeratorClass.AddProperty(L"Current", whereEnumerator_T, LINK_INSTANCE)
            .AddGetter()
            .IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET)
            .SetCallback(&shard_whereenumerator_Current_get);
    }

    SymbolBuilder<MethodSymbol> whereMethod = linqNamespace.AddMethod(L"Where", TYPE_ANY, LINK_STATIC, ACS_PUBLIC);
    TypeParameterSymbol* where_T = whereMethod.AddTypeParameter(L"T").Get();
    whereMethod.Get()->ReturnType = factory.GenericType(TRAIT_ENUMERABLE, { { L"T", where_T } });
    whereMethod
        .AddParameter(L"source", factory.GenericType(TRAIT_ENUMERABLE, { { L"T", where_T } }))
        .AddParameter(L"predicate", factory.GenericType(funcDelegate, { { L"T", where_T }, { L"U", TYPE_BOOL } }))
        .SetCallback(&shard_linq_Where);

    // ------------------------------------------------------------------------
    // OrderBy<T, TKey>
    // ------------------------------------------------------------------------
    {
        SymbolBuilder<ClassSymbol> orderByEnumerableClass = linqNamespace.AddClass(L"OrderByEnumerable");
        TypeParameterSymbol* orderByEnumerable_T = orderByEnumerableClass.AddTypeParameter(L"T").Get();
        TypeParameterSymbol* orderByEnumerable_TKey = orderByEnumerableClass.AddTypeParameter(L"TKey").Get();

        orderByEnumerableClass_raw = orderByEnumerableClass.Get();

        orderByEnumerableClass
            .Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", orderByEnumerable_T } }));

        orderByEnumerable_sortedField = orderByEnumerableClass
            .AddField(L"_sorted", factory.Array(orderByEnumerable_T), LINK_INSTANCE, ACS_PRIVATE).Get();

        orderByEnumerableClass.AddMethod(L"GetEnumerator",
            factory.GenericType(TRAIT_ENUMERATOR, { { L"T", orderByEnumerable_T } }), LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
            .SetCallback(&shard_orderbyenumerable_GetEnumerator);
    }

    {
        SymbolBuilder<ClassSymbol> orderByEnumeratorClass = linqNamespace.AddClass(L"OrderByEnumerator");
        TypeParameterSymbol* orderByEnumerator_T = orderByEnumeratorClass.AddTypeParameter(L"T").Get();

        orderByEnumeratorClass_raw = orderByEnumeratorClass.Get();

        orderByEnumeratorClass
            .Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", orderByEnumerator_T } }));

        orderByEnumerator_sortedField = orderByEnumeratorClass
            .AddField(L"_sorted", factory.Array(orderByEnumerator_T), LINK_INSTANCE, ACS_PRIVATE).Get();

        orderByEnumerator_indexField = orderByEnumeratorClass
            .AddField(L"_index", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE).Get();

        orderByEnumeratorClass.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE)
            .IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
            .SetCallback(&shard_orderbyenumerator_MoveNext);

        orderByEnumeratorClass.AddProperty(L"Current", orderByEnumerator_T, LINK_INSTANCE)
            .AddGetter()
            .IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET)
            .SetCallback(&shard_orderbyenumerator_Current_get);
    }

    SymbolBuilder<MethodSymbol> orderByMethod = linqNamespace.AddMethod(L"OrderBy", TYPE_ANY, LINK_STATIC, ACS_PUBLIC);
    TypeParameterSymbol* orderBy_T = orderByMethod.AddTypeParameter(L"T").Get();
    TypeParameterSymbol* orderBy_TKey = orderByMethod.AddTypeParameter(L"TKey").Get();
    orderByMethod.Get()->ReturnType = factory.GenericType(TRAIT_ENUMERABLE, { { L"T", orderBy_T } });
    orderByMethod
        .AddParameter(L"source", factory.GenericType(TRAIT_ENUMERABLE, { { L"T", orderBy_T } }))
        .AddParameter(L"keySelector", factory.GenericType(funcDelegate, { { L"T", orderBy_T }, { L"U", orderBy_TKey } }))
        .SetCallback(&shard_linq_OrderBy);
}
