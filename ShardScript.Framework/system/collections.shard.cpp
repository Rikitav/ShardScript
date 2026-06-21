#include <stdexcept>
#include <string>

#include <ShardScript.hpp>

using namespace shard;

static FieldSymbol* list_arrayField = nullptr;

static ObjectInstance* shard_list_init(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ArrayTypeSymbol* arrayType = new ArrayTypeSymbol(concreteT);
	arrayType->Size = 0;
	arrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	arrayType->State = TypeLayoutingState::Visited;

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
	arrayType->Size = 0;
	arrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	arrayType->State = TypeLayoutingState::Visited;

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

	std::size_t currentSize = arrayType->Size;
	std::size_t newSize = currentSize + 1;

	ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
	newArrayType->Size = newSize;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
	newArrayType->State = TypeLayoutingState::Visited;

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

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Size)
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

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Size)
		throw std::runtime_error("index is out of bounds");

	std::size_t newSize = arrayType->Size - 1;

	ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(concreteT);
	newArrayType->Size = newSize;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize + concreteT->GetInlineSize() * newSize;
	newArrayType->State = TypeLayoutingState::Visited;

	ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);

	for (std::size_t i = 0; i < static_cast<std::size_t>(index); i++)
	{
		ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
		newArray->SetElement(i, element, context.Frame);
	}

	for (std::size_t i = static_cast<std::size_t>(index) + 1; i < arrayType->Size; i++)
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
	newArrayType->Size = 0;
	newArrayType->MemoryBytesSize = SymbolTable::Primitives::Array->MemoryBytesSize;
	newArrayType->State = TypeLayoutingState::Visited;

	ObjectInstance* newArray = context.Collector.AllocateInstance(newArrayType);
	listInstance->SetField(list_arrayField, newArray, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_Length_get(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	return context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Size));
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

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Size)
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

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Size)
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

	// --- class List<T> ---
	SymbolBuilder<ClassSymbol> listClass = collectionsNs.AddClass(L"List");

	// T
	TypeParameterSymbol* typeParamT = listClass
		.AddTypeParameter(L"T");
	
	// List<T>._array: T[]
	list_arrayField = listClass
		.AddField(L"_array", listClass.GetFactory().Array(typeParamT), LINK_INSTANCE, ACS_PRIVATE).Get();

	listClass.AddInit()
		.SetCallback(&shard_list_init);

	// List<T>.Length -> int { get; }
	SymbolBuilder<PropertySymbol> lengthProp = listClass.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE);
	lengthProp.AddGetter()
			.SetCallback(&shard_list_Length_get);

	// List<T>[index: int] -> T { get; set; }
	SymbolBuilder<IndexatorSymbol> indexer = listClass.AddIndexer(typeParamT, LINK_INSTANCE);
	indexer.AddGetter()
		.SetCallback(&shard_list_Indexer_get);

	indexer.AddSetter()
		.SetCallback(&shard_list_Indexer_set);

	// List<T>.Add(item: T) -> void
	SymbolBuilder<MethodSymbol> addMethod = listClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE);
	addMethod
		.AddParameter(L"item", typeParamT)
		.SetCallback(&shard_list_Add);

	// List<T>.ElementAt(index: int) -> T
	SymbolBuilder<MethodSymbol> elementAtMethod = listClass.AddMethod(L"ElementAt", typeParamT, LINK_INSTANCE);
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
}
