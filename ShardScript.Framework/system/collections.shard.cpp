#include <stdexcept>
#include <string>

#include <ShardScript.hpp>

using namespace shard;

static FieldSymbol* list_arrayField = nullptr;
static StructSymbol* listEnumeratorClass_raw = nullptr;
static TypeParameterSymbol* listEnumerator_typeParam_T = nullptr;
static FieldSymbol* listEnumerator_sourceField = nullptr;
static FieldSymbol* listEnumerator_indexField = nullptr;
static FieldSymbol* listEnumerator_lengthField = nullptr;

static ObjectInstance* shard_list_init(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(concreteT);
	arrayType->Length = 0;
	arrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	arrayType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* array = context.Collector.AllocateInstance(arrayType);
	listInstance->SetField(list_arrayField, array, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_init_capacity(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(concreteT);
	arrayType->Length = 0;
	arrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	arrayType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* array = context.Collector.AllocateInstance(arrayType);
	listInstance->SetField(list_arrayField, array, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_Add(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	ObjectInstance* value = context.Args[1];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	std::size_t currentSize = arrayType->Length;
	std::size_t newSize = currentSize + 1;

	ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
	newArrayType->Length = newSize;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
	newArrayType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);

	for (std::size_t i = 0; i < currentSize; i++)
	{
		ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
		newArray->SetElement(i, element, context.Frame);
	}

	newArray->SetElement(currentSize, value, context.Frame);
	listInstance->SetField(list_arrayField, newArray, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_ElementAt(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	return arrayInstance->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* shard_list_RemoveAt(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	std::size_t newSize = arrayType->Length - 1;

	ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
	newArrayType->Length = newSize;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
	newArrayType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);

	for (std::size_t i = 0; i < static_cast<std::size_t>(index); i++)
	{
		ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
		newArray->SetElement(i, element, context.Frame);
	}

	for (std::size_t i = static_cast<std::size_t>(index) + 1; i < arrayType->Length; i++)
	{
		ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
		newArray->SetElement(i - 1, element, context.Frame);
	}

	listInstance->SetField(list_arrayField, newArray, context.Frame);
	return nullptr;
}

static ObjectInstance* shard_list_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
	newArrayType->Length = 0;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	newArrayType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);
	listInstance->SetField(list_arrayField, newArray, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_listenumerator_MoveNext(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(listEnumerator_indexField, context.Frame)->AsInteger();
	std::int64_t length = self->GetField(listEnumerator_lengthField, context.Frame)->AsInteger();

	index++;
	self->SetField(listEnumerator_indexField, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(index < length);
}

static ObjectInstance* shard_listenumerator_Current_get(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(listEnumerator_indexField, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(listEnumerator_sourceField, context.Frame);
	return source->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* shard_list_GetEnumerator(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];
	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(arrayInstance->getInfo());

	GenericTypeSymbol* enumeratorType = new GenericTypeSymbol(listEnumeratorClass_raw);
	enumeratorType->AddTypeParameter(listEnumerator_typeParam_T, concreteT);
	enumeratorType->Inlining = TypeInlining::ByReference;
	enumeratorType->MemoryBytesSize = listEnumeratorClass_raw->MemoryBytesSize;
	enumeratorType->LayoutingState = TypeLayoutingState::Visited;

	ObjectInstance* enumerator = context.Collector.AllocateInstance(enumeratorType);
	enumerator->SetField(listEnumerator_sourceField, arrayInstance, context.Frame);
	enumerator->SetField(listEnumerator_indexField, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	enumerator->SetField(listEnumerator_lengthField, context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length)), context.Frame);

	return enumerator;
}

static ObjectInstance* shard_list_Length_get(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	return context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length));
}

static ObjectInstance* shard_list_Indexer_get(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	return arrayInstance->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* shard_list_Indexer_set(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();
	ObjectInstance* value = context.Args[2];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	arrayInstance->SetElement(static_cast<std::size_t>(index), value, context.Frame);
	return nullptr;
}

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.collections";
	lib.Description = L"Standard collections";
	lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> collectionsNs(context, L"collections");
	SymbolFactory factory(context.GetSemanticModel().Table.get());

	// --- class ListEnumerator<T> ---
	SymbolBuilder<StructSymbol> listEnumeratorClass = collectionsNs.AddStruct(L"ListEnumerator");

	// T
	TypeParameterSymbol* listEnumeratorClass_typeParam_T = listEnumeratorClass
		.AddTypeParameter(L"T");

	listEnumeratorClass_raw = listEnumeratorClass.Get();
	listEnumerator_typeParam_T = listEnumeratorClass_typeParam_T;

	listEnumeratorClass
		.Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", listEnumeratorClass_typeParam_T } }));

	listEnumerator_sourceField = listEnumeratorClass
		.AddField(L"_source", listEnumeratorClass.GetFactory().Array(listEnumeratorClass_typeParam_T), LINK_INSTANCE, ACS_PRIVATE).Get();

	listEnumerator_indexField = listEnumeratorClass
		.AddField(L"_index", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	listEnumerator_lengthField = listEnumeratorClass
		.AddField(L"_length", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	listEnumeratorClass.AddMethod(L"MoveNext", SymbolTable::Primitives::Boolean, LINK_INSTANCE)
		.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
		.SetCallback(&shard_listenumerator_MoveNext);

	SymbolBuilder<PropertySymbol> currentProp = listEnumeratorClass.AddProperty(L"Current", listEnumeratorClass_typeParam_T, LINK_INSTANCE);
	AccessorSymbol* currentGetter = currentProp.AddGetter()
		.SetCallback(&shard_listenumerator_Current_get)
		.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET);

	// --- class List<T> ---
	SymbolBuilder<ClassSymbol> listClass = collectionsNs.AddClass(L"List");

	// T
	TypeParameterSymbol* listClass_typeParam_T = listClass
		.AddTypeParameter(L"T");

	listClass.Implements(
		factory.GenericType(TRAIT_ENUMERABLE, { { L"T", listClass_typeParam_T } }));

	// List<T>._array: T[]
	list_arrayField = listClass
		.AddField(L"_array", listClass.GetFactory().Array(listClass_typeParam_T), LINK_INSTANCE, ACS_PRIVATE).Get();

	listClass.AddInit()
		.SetCallback(&shard_list_init);

	// List<T>.Length -> int { get; }
	SymbolBuilder<PropertySymbol> lengthProp = listClass.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE);
	lengthProp.AddGetter()
		.SetCallback(&shard_list_Length_get);

	// List<T>[index: int] -> T { get; set; }
	SymbolBuilder<IndexatorSymbol> indexer = listClass.AddIndexer(listClass_typeParam_T, LINK_INSTANCE);
	indexer.AddGetter()
		.SetCallback(&shard_list_Indexer_get);

	indexer.AddSetter()
		.SetCallback(&shard_list_Indexer_set);

	// List<T>.Add(item: T) -> void
	SymbolBuilder<MethodSymbol> addMethod = listClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE);
	addMethod
		.AddParameter(L"item", listClass_typeParam_T)
		.SetCallback(&shard_list_Add);

	// List<T>.ElementAt(index: int) -> T
	SymbolBuilder<MethodSymbol> elementAtMethod = listClass.AddMethod(L"ElementAt", listClass_typeParam_T, LINK_INSTANCE);
	elementAtMethod
		.AddParameter(L"index", TYPE_INT)
		.SetCallback(&shard_list_ElementAt);

	// List<T>.RemoveAt(index: int) -> void
	SymbolBuilder<MethodSymbol> removeAtMethod = listClass.AddMethod(L"RemoveAt", TYPE_VOID, LINK_INSTANCE);
	removeAtMethod
		.AddParameter(L"index", TYPE_INT)
		.SetCallback(&shard_list_RemoveAt);

	// List<T>.Clear() -> void
	SymbolBuilder<MethodSymbol> clearMethod = listClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE);
	clearMethod
		.SetCallback(&shard_list_Clear);

	// List<T>.GetEnumerator() -> IEnumerator<T>
	SymbolBuilder<MethodSymbol> getEnumeratorMethod = listClass.AddMethod(
		L"GetEnumerator",
		factory.GenericType(TRAIT_ENUMERATOR, { { L"T", listClass_typeParam_T } }),
		LINK_INSTANCE);

	getEnumeratorMethod
		.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
		.SetCallback(&shard_list_GetEnumerator);
}
