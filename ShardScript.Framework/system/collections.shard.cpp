#include <stdexcept>
#include <string>
#include <cwchar>
#include <cstring>
#include <limits>

#include <ShardScript.hpp>
#include <shard/semantic/SemanticModel.hpp>

using namespace shard;

// =========================================================================
//  List<T> / ListEnumerator<T>
// =========================================================================
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

	ObjectInstance* array = context.Collector.AllocateArray(concreteT, 0);
	listInstance->SetField(list_arrayField->SlotIndex, array, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_init_capacity(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ObjectInstance* array = context.Collector.AllocateArray(concreteT, 0);
	listInstance->SetField(list_arrayField->SlotIndex, array, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_Add(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	ObjectInstance* value = context.Args[1];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	std::size_t currentSize = arrayType->Length;
	std::size_t newSize = currentSize + 1;

	ObjectInstance* newArray = context.Collector.AllocateArray(concreteT, newSize);

	for (std::size_t i = 0; i < currentSize; i++)
	{
		ObjectInstance* element = arrayInstance->GetElement(i, context.Frame);
		newArray->SetElement(i, element, context.Frame);
	}

	newArray->SetElement(currentSize, value, context.Frame);
	listInstance->SetField(list_arrayField->SlotIndex, newArray, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_list_ElementAt(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
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

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));
	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	std::size_t newSize = arrayType->Length - 1;

	ObjectInstance* newArray = context.Collector.AllocateArray(concreteT, newSize);

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

	listInstance->SetField(list_arrayField->SlotIndex, newArray, context.Frame);
	return nullptr;
}

static ObjectInstance* shard_list_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];

	ObjectInstance* newArray = context.Collector.AllocateArray(concreteT, 0);
	listInstance->SetField(list_arrayField->SlotIndex, newArray, context.Frame);

	return nullptr;
}

static ObjectInstance* shard_listenumerator_MoveNext(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(listEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	std::int64_t length = self->GetField(listEnumerator_lengthField->SlotIndex, context.Frame)->AsInteger();

	index++;
	self->SetField(listEnumerator_indexField->SlotIndex, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(index < length);
}

static ObjectInstance* shard_listenumerator_Current_get(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(listEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(listEnumerator_sourceField->SlotIndex, context.Frame);
	return source->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* shard_list_GetEnumerator(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	TypeSymbol* concreteT = context.Frame->TypeArguments[0];
	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
	const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(arrayInstance->getInfo());

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(listEnumeratorClass_raw, std::vector<TypeSymbol*>{ concreteT });
	enumerator->SetField(listEnumerator_sourceField->SlotIndex, arrayInstance, context.Frame);
	enumerator->SetField(listEnumerator_indexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	enumerator->SetField(listEnumerator_lengthField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length)), context.Frame);

	return enumerator;
}

static ObjectInstance* shard_list_Length_get(const CallState& context) noexcept(false)
{
	ObjectInstance* listInstance = context.Args[0];

	TypeSymbol* ownerType = const_cast<TypeSymbol*>(listInstance->getInfo());
	if (ownerType->Kind == SyntaxKind::GenericType)
		ownerType = static_cast<GenericTypeSymbol*>(ownerType)->UnderlayingType;

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
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

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
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

	ObjectInstance* arrayInstance = listInstance->GetField(list_arrayField->SlotIndex, context.Frame);
	ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(const_cast<TypeSymbol*>(arrayInstance->getInfo()));

	if (index < 0 || static_cast<std::size_t>(index) >= arrayType->Length)
		throw std::runtime_error("index is out of bounds");

	arrayInstance->SetElement(static_cast<std::size_t>(index), value, context.Frame);
	return nullptr;
}

// =========================================================================
//  KeyValuePair<K, V>
// =========================================================================
static StructSymbol* keyValuePairClass_raw = nullptr;
static TypeParameterSymbol* keyValuePair_typeParam_K = nullptr;
static TypeParameterSymbol* keyValuePair_typeParam_V = nullptr;
static FieldSymbol* kvp_keyField = nullptr;
static FieldSymbol* kvp_valueField = nullptr;

// =========================================================================
//  Dictionary<K, V>
// =========================================================================
static ClassSymbol* dictionaryClass_raw = nullptr;
static TypeParameterSymbol* dictionary_typeParam_K = nullptr;
static TypeParameterSymbol* dictionary_typeParam_V = nullptr;
static FieldSymbol* dict_keysField = nullptr;
static FieldSymbol* dict_valuesField = nullptr;
static FieldSymbol* dict_hashesField = nullptr;
static FieldSymbol* dict_statesField = nullptr;
static FieldSymbol* dict_countField = nullptr;

static StructSymbol* dictionaryEnumeratorClass_raw = nullptr;
static TypeParameterSymbol* dictEnumerator_typeParam_K = nullptr;
static TypeParameterSymbol* dictEnumerator_typeParam_V = nullptr;
static FieldSymbol* dictEnumerator_sourceField = nullptr;
static FieldSymbol* dictEnumerator_indexField = nullptr;

// =========================================================================
//  Queue<T>
// =========================================================================
static ClassSymbol* queueClass_raw = nullptr;
static TypeParameterSymbol* queue_typeParam_T = nullptr;
static FieldSymbol* queue_arrayField = nullptr;
static FieldSymbol* queue_headField = nullptr;
static FieldSymbol* queue_tailField = nullptr;
static FieldSymbol* queue_sizeField = nullptr;

static StructSymbol* queueEnumeratorClass_raw = nullptr;
static TypeParameterSymbol* queueEnumerator_typeParam_T = nullptr;
static FieldSymbol* queueEnumerator_sourceField = nullptr;
static FieldSymbol* queueEnumerator_indexField = nullptr;

// =========================================================================
//  Stack<T>
// =========================================================================
static ClassSymbol* stackClass_raw = nullptr;
static TypeParameterSymbol* stack_typeParam_T = nullptr;
static FieldSymbol* stack_arrayField = nullptr;
static FieldSymbol* stack_sizeField = nullptr;

static StructSymbol* stackEnumeratorClass_raw = nullptr;
static TypeParameterSymbol* stackEnumerator_typeParam_T = nullptr;
static FieldSymbol* stackEnumerator_sourceField = nullptr;
static FieldSymbol* stackEnumerator_indexField = nullptr;

// =========================================================================
//  Equality / hashing helpers
// =========================================================================
static std::int64_t GetObjectHash(ObjectInstance* value)
{
	if (value == nullptr)
		return 0;

	const TypeSymbol* info = value->getInfo();

	if (info == SymbolTable::Primitives::Integer
		|| info == SymbolTable::Primitives::Char
		|| info == SymbolTable::Primitives::Boolean)
	{
		return value->AsInteger();
	}

	if (info == SymbolTable::Primitives::Double)
	{
		double d = value->AsDouble();
		std::int64_t h = 0;
		static_assert(sizeof(h) == sizeof(d), "double and int64 sizes must match");
		std::memcpy(&h, &d, sizeof(d));
		return h;
	}

	if (info == SymbolTable::Primitives::String)
	{
		const wchar_t* s = value->AsString();
		std::int64_t len = value->AsStringLength();
		std::int64_t hash = 5381;
		for (std::int64_t i = 0; i < len; ++i)
			hash = ((hash << 5) + hash) + s[i];
		return hash;
	}

	std::int64_t h = 0;
	std::memcpy(&h, &value, sizeof(value));
	return h;
}

static bool ObjectsEqual(ObjectInstance* a, ObjectInstance* b)
{
	if (a == b)
		return true;
	if (a == nullptr || b == nullptr)
		return false;

	const TypeSymbol* infoA = a->getInfo();
	const TypeSymbol* infoB = b->getInfo();
	if (!SemanticModel::AreTypesEqual(infoA, infoB))
		return false;

	if (infoA == SymbolTable::Primitives::Integer
		|| infoA == SymbolTable::Primitives::Char
		|| infoA == SymbolTable::Primitives::Boolean)
	{
		return a->AsInteger() == b->AsInteger();
	}

	if (infoA == SymbolTable::Primitives::Double)
		return a->AsDouble() == b->AsDouble();

	if (infoA == SymbolTable::Primitives::String)
	{
		std::int64_t lenA = a->AsStringLength();
		if (lenA != b->AsStringLength())
			return false;
		return std::wmemcmp(a->AsString(), b->AsString(), static_cast<std::size_t>(lenA)) == 0;
	}

	return a == b;
}

// =========================================================================
//  Dictionary<K, V> implementation
// =========================================================================
static std::size_t dictionary_Capacity(ObjectInstance* dict, CallStackFrame* frame)
{
	ObjectInstance* keys = dict->GetField(dict_keysField->SlotIndex, frame);
	if (keys == nullptr)
		return 0;
	return keys->GetArrayLength();
}

static void dictionary_Resize(
	ObjectInstance* dict,
	TypeSymbol* keyType,
	TypeSymbol* valueType,
	std::size_t newCapacity,
	CallStackFrame* frame,
	GarbageCollector& collector)
{
	ObjectInstance* oldKeys = dict->GetField(dict_keysField->SlotIndex, frame);
	ObjectInstance* oldValues = dict->GetField(dict_valuesField->SlotIndex, frame);
	ObjectInstance* oldHashes = dict->GetField(dict_hashesField->SlotIndex, frame);
	ObjectInstance* oldStates = dict->GetField(dict_statesField->SlotIndex, frame);
	std::size_t oldCapacity = oldKeys ? oldKeys->GetArrayLength() : 0;

	ObjectInstance* newKeys = collector.AllocateArray(keyType, newCapacity);
	ObjectInstance* newValues = collector.AllocateArray(valueType, newCapacity);
	ObjectInstance* newHashes = collector.AllocateArray(SymbolTable::Primitives::Integer, newCapacity);
	ObjectInstance* newStates = collector.AllocateArray(SymbolTable::Primitives::Integer, newCapacity);

	for (std::size_t i = 0; i < newCapacity; ++i)
		newStates->SetElement(i, collector.FromValue(static_cast<std::int64_t>(0)), frame);

	for (std::size_t i = 0; i < oldCapacity; ++i)
	{
		std::int64_t state = oldStates->GetElement(i, frame)->AsInteger();
		if (state != 1)
			continue;

		ObjectInstance* key = oldKeys->GetElement(i, frame);
		ObjectInstance* value = oldValues->GetElement(i, frame);
		std::int64_t hash = oldHashes->GetElement(i, frame)->AsInteger();

		std::size_t index = static_cast<std::size_t>((hash % static_cast<std::int64_t>(newCapacity) + static_cast<std::int64_t>(newCapacity)) % static_cast<std::int64_t>(newCapacity));
		for (std::size_t j = 0; j < newCapacity; ++j)
		{
			std::size_t idx = (index + j) % newCapacity;
			if (newStates->GetElement(idx, frame)->AsInteger() != 0)
				continue;

			newHashes->SetElement(idx, collector.FromValue(hash), frame);
			newStates->SetElement(idx, collector.FromValue(static_cast<std::int64_t>(1)), frame);
			newKeys->SetElement(idx, key, frame);
			newValues->SetElement(idx, value, frame);
			break;
		}
	}

	dict->SetField(dict_keysField->SlotIndex, newKeys, frame);
	dict->SetField(dict_valuesField->SlotIndex, newValues, frame);
	dict->SetField(dict_hashesField->SlotIndex, newHashes, frame);
	dict->SetField(dict_statesField->SlotIndex, newStates, frame);
}

static void dictionary_EnsureCapacity(
	ObjectInstance* dict,
	TypeSymbol* keyType,
	TypeSymbol* valueType,
	CallStackFrame* frame,
	GarbageCollector& collector)
{
	std::size_t capacity = dictionary_Capacity(dict, frame);
	std::int64_t count = dict->GetField(dict_countField->SlotIndex, frame)->AsInteger();

	if (capacity == 0)
	{
		dictionary_Resize(dict, keyType, valueType, 4, frame, collector);
		return;
	}

	if (static_cast<std::size_t>(count) + 1 > (capacity * 3) / 4)
		dictionary_Resize(dict, keyType, valueType, capacity * 2, frame, collector);
}

static std::size_t dictionary_FindSlot(
	ObjectInstance* dict,
	ObjectInstance* key,
	bool& found,
	CallStackFrame* frame)
{
	found = false;
	std::size_t capacity = dictionary_Capacity(dict, frame);
	if (capacity == 0)
		return std::numeric_limits<std::size_t>::max();

	ObjectInstance* keys = dict->GetField(dict_keysField->SlotIndex, frame);
	ObjectInstance* hashes = dict->GetField(dict_hashesField->SlotIndex, frame);
	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, frame);

	std::int64_t hash = GetObjectHash(key);
	std::size_t index = static_cast<std::size_t>((hash % static_cast<std::int64_t>(capacity) + static_cast<std::int64_t>(capacity)) % static_cast<std::int64_t>(capacity));

	std::size_t firstDeleted = std::numeric_limits<std::size_t>::max();
	for (std::size_t i = 0; i < capacity; ++i)
	{
		std::size_t idx = (index + i) % capacity;
		std::int64_t state = states->GetElement(idx, frame)->AsInteger();

		if (state == 0)
			return firstDeleted != std::numeric_limits<std::size_t>::max() ? firstDeleted : idx;

		if (state == -1 && firstDeleted == std::numeric_limits<std::size_t>::max())
			firstDeleted = idx;

		if (state == 1 && hashes->GetElement(idx, frame)->AsInteger() == hash && ObjectsEqual(keys->GetElement(idx, frame), key))
		{
			found = true;
			return idx;
		}
	}

	return firstDeleted != std::numeric_limits<std::size_t>::max() ? firstDeleted : std::numeric_limits<std::size_t>::max();
}

static ObjectInstance* shard_dict_init(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	ObjectInstance* keys = context.Collector.AllocateArray(keyType, 0);
	ObjectInstance* values = context.Collector.AllocateArray(valueType, 0);
	ObjectInstance* hashes = context.Collector.AllocateArray(SymbolTable::Primitives::Integer, 0);
	ObjectInstance* states = context.Collector.AllocateArray(SymbolTable::Primitives::Integer, 0);

	dict->SetField(dict_keysField->SlotIndex, keys, context.Frame);
	dict->SetField(dict_valuesField->SlotIndex, values, context.Frame);
	dict->SetField(dict_hashesField->SlotIndex, hashes, context.Frame);
	dict->SetField(dict_statesField->SlotIndex, states, context.Frame);
	dict->SetField(dict_countField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_dict_Add(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	ObjectInstance* key = context.Args[1];
	ObjectInstance* value = context.Args[2];

	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	dictionary_EnsureCapacity(dict, keyType, valueType, context.Frame, context.Collector);

	bool found = false;
	std::size_t slot = dictionary_FindSlot(dict, key, found, context.Frame);
	if (found)
		throw std::runtime_error("dictionary already contains the given key");

	ObjectInstance* keys = dict->GetField(dict_keysField->SlotIndex, context.Frame);
	ObjectInstance* values = dict->GetField(dict_valuesField->SlotIndex, context.Frame);
	ObjectInstance* hashes = dict->GetField(dict_hashesField->SlotIndex, context.Frame);
	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, context.Frame);

	hashes->SetElement(slot, context.Collector.FromValue(GetObjectHash(key)), context.Frame);
	states->SetElement(slot, context.Collector.FromValue(static_cast<std::int64_t>(1)), context.Frame);
	keys->SetElement(slot, key, context.Frame);
	values->SetElement(slot, value, context.Frame);

	std::int64_t count = dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger();
	dict->SetField(dict_countField->SlotIndex, context.Collector.FromValue(count + 1), context.Frame);

	return nullptr;
}

static ObjectInstance* shard_dict_Indexer_get(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	ObjectInstance* key = context.Args[1];

	bool found = false;
	std::size_t slot = dictionary_FindSlot(dict, key, found, context.Frame);
	if (!found)
		throw std::runtime_error("key not found in dictionary");

	ObjectInstance* values = dict->GetField(dict_valuesField->SlotIndex, context.Frame);
	return values->GetElement(slot, context.Frame);
}

static ObjectInstance* shard_dict_Indexer_set(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	ObjectInstance* key = context.Args[1];
	ObjectInstance* value = context.Args[2];

	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	dictionary_EnsureCapacity(dict, keyType, valueType, context.Frame, context.Collector);

	bool found = false;
	std::size_t slot = dictionary_FindSlot(dict, key, found, context.Frame);

	ObjectInstance* keys = dict->GetField(dict_keysField->SlotIndex, context.Frame);
	ObjectInstance* values = dict->GetField(dict_valuesField->SlotIndex, context.Frame);
	ObjectInstance* hashes = dict->GetField(dict_hashesField->SlotIndex, context.Frame);
	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, context.Frame);

	if (found)
	{
		values->SetElement(slot, value, context.Frame);
		return nullptr;
	}

	hashes->SetElement(slot, context.Collector.FromValue(GetObjectHash(key)), context.Frame);
	states->SetElement(slot, context.Collector.FromValue(static_cast<std::int64_t>(1)), context.Frame);
	keys->SetElement(slot, key, context.Frame);
	values->SetElement(slot, value, context.Frame);

	std::int64_t count = dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger();
	dict->SetField(dict_countField->SlotIndex, context.Collector.FromValue(count + 1), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_dict_ContainsKey(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	ObjectInstance* key = context.Args[1];

	bool found = false;
	dictionary_FindSlot(dict, key, found, context.Frame);
	return context.Collector.FromValue(found);
}

static ObjectInstance* shard_dict_Remove(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	ObjectInstance* key = context.Args[1];

	bool found = false;
	std::size_t slot = dictionary_FindSlot(dict, key, found, context.Frame);
	if (!found)
		return context.Collector.FromValue(false);

	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, context.Frame);
	states->SetElement(slot, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);

	std::int64_t count = dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger();
	dict->SetField(dict_countField->SlotIndex, context.Collector.FromValue(count - 1), context.Frame);
	return context.Collector.FromValue(true);
}

static ObjectInstance* shard_dict_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	ObjectInstance* keys = context.Collector.AllocateArray(keyType, 0);
	ObjectInstance* values = context.Collector.AllocateArray(valueType, 0);
	ObjectInstance* hashes = context.Collector.AllocateArray(SymbolTable::Primitives::Integer, 0);
	ObjectInstance* states = context.Collector.AllocateArray(SymbolTable::Primitives::Integer, 0);

	dict->SetField(dict_keysField->SlotIndex, keys, context.Frame);
	dict->SetField(dict_valuesField->SlotIndex, values, context.Frame);
	dict->SetField(dict_hashesField->SlotIndex, hashes, context.Frame);
	dict->SetField(dict_statesField->SlotIndex, states, context.Frame);
	dict->SetField(dict_countField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_dict_Count_get(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	return context.Collector.FromValue(dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger());
}

static ObjectInstance* shard_dict_Keys_get(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	TypeSymbol* keyType = context.Frame->TypeArguments[0];

	ObjectInstance* keys = dict->GetField(dict_keysField->SlotIndex, context.Frame);
	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, context.Frame);
	std::size_t capacity = keys ? keys->GetArrayLength() : 0;
	std::int64_t count = dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger();

	ObjectInstance* result = context.Collector.AllocateArray(keyType, static_cast<std::size_t>(count));
	std::size_t resultIndex = 0;
	for (std::size_t i = 0; i < capacity; ++i)
	{
		if (states->GetElement(i, context.Frame)->AsInteger() == 1)
		{
			result->SetElement(resultIndex, keys->GetElement(i, context.Frame), context.Frame);
			++resultIndex;
		}
	}

	return result;
}

static ObjectInstance* shard_dict_Values_get(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	ObjectInstance* values = dict->GetField(dict_valuesField->SlotIndex, context.Frame);
	ObjectInstance* states = dict->GetField(dict_statesField->SlotIndex, context.Frame);
	std::size_t capacity = values ? values->GetArrayLength() : 0;
	std::int64_t count = dict->GetField(dict_countField->SlotIndex, context.Frame)->AsInteger();

	ObjectInstance* result = context.Collector.AllocateArray(valueType, static_cast<std::size_t>(count));
	std::size_t resultIndex = 0;
	for (std::size_t i = 0; i < capacity; ++i)
	{
		if (states->GetElement(i, context.Frame)->AsInteger() == 1)
		{
			result->SetElement(resultIndex, values->GetElement(i, context.Frame), context.Frame);
			++resultIndex;
		}
	}

	return result;
}

static ObjectInstance* shard_dict_GetEnumerator(const CallState& context) noexcept(false)
{
	ObjectInstance* dict = context.Args[0];
	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(dictionaryEnumeratorClass_raw, std::vector<TypeSymbol*>{ keyType, valueType });
	enumerator->SetField(dictEnumerator_sourceField->SlotIndex, dict, context.Frame);
	enumerator->SetField(dictEnumerator_indexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	return enumerator;
}

static ObjectInstance* shard_dictionaryenumerator_MoveNext(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	ObjectInstance* source = self->GetField(dictEnumerator_sourceField->SlotIndex, context.Frame);
	ObjectInstance* states = source->GetField(dict_statesField->SlotIndex, context.Frame);
	std::size_t capacity = states ? states->GetArrayLength() : 0;

	std::int64_t index = self->GetField(dictEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	++index;

	while (static_cast<std::size_t>(index) < capacity)
	{
		if (states->GetElement(static_cast<std::size_t>(index), context.Frame)->AsInteger() == 1)
		{
			self->SetField(dictEnumerator_indexField->SlotIndex, context.Collector.FromValue(index), context.Frame);
			return context.Collector.FromValue(true);
		}
		++index;
	}

	self->SetField(dictEnumerator_indexField->SlotIndex, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(false);
}

static ObjectInstance* shard_dictionaryenumerator_Current_get(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(dictEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(dictEnumerator_sourceField->SlotIndex, context.Frame);

	ObjectInstance* keys = source->GetField(dict_keysField->SlotIndex, context.Frame);
	ObjectInstance* values = source->GetField(dict_valuesField->SlotIndex, context.Frame);

	TypeSymbol* keyType = context.Frame->TypeArguments[0];
	TypeSymbol* valueType = context.Frame->TypeArguments[1];

	ObjectInstance* pair = context.Collector.AllocateGeneric(keyValuePairClass_raw, std::vector<TypeSymbol*>{ keyType, valueType });
	pair->SetField(kvp_keyField->SlotIndex, keys->GetElement(static_cast<std::size_t>(index), context.Frame), context.Frame);
	pair->SetField(kvp_valueField->SlotIndex, values->GetElement(static_cast<std::size_t>(index), context.Frame), context.Frame);
	return pair;
}

// =========================================================================
//  Queue<T> implementation
// =========================================================================
static void queue_EnsureCapacity(
	ObjectInstance* queue,
	TypeSymbol* elementType,
	CallStackFrame* frame,
	GarbageCollector& collector)
{
	ObjectInstance* array = queue->GetField(queue_arrayField->SlotIndex, frame);
	std::int64_t size = queue->GetField(queue_sizeField->SlotIndex, frame)->AsInteger();
	std::size_t capacity = array ? array->GetArrayLength() : 0;

	if (static_cast<std::size_t>(size) < capacity)
		return;

	std::size_t newCapacity = capacity == 0 ? 4 : capacity * 2;
	ObjectInstance* newArray = collector.AllocateArray(elementType, newCapacity);

	std::int64_t head = queue->GetField(queue_headField->SlotIndex, frame)->AsInteger();
	for (std::int64_t i = 0; i < size; ++i)
	{
		std::size_t oldIdx = static_cast<std::size_t>((head + i) % static_cast<std::int64_t>(capacity));
		newArray->SetElement(static_cast<std::size_t>(i), array->GetElement(oldIdx, frame), frame);
	}

	queue->SetField(queue_arrayField->SlotIndex, newArray, frame);
	queue->SetField(queue_headField->SlotIndex, collector.FromValue(static_cast<std::int64_t>(0)), frame);
	queue->SetField(queue_tailField->SlotIndex, collector.FromValue(size), frame);
}

static ObjectInstance* shard_queue_init(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	queue->SetField(queue_arrayField->SlotIndex, context.Collector.AllocateArray(elementType, 0), context.Frame);
	queue->SetField(queue_headField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	queue->SetField(queue_tailField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	queue->SetField(queue_sizeField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_queue_Enqueue(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	ObjectInstance* value = context.Args[1];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	queue_EnsureCapacity(queue, elementType, context.Frame, context.Collector);

	ObjectInstance* array = queue->GetField(queue_arrayField->SlotIndex, context.Frame);
	std::size_t capacity = array->GetArrayLength();
	std::int64_t tail = queue->GetField(queue_tailField->SlotIndex, context.Frame)->AsInteger();
	std::int64_t size = queue->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger();

	array->SetElement(static_cast<std::size_t>(tail), value, context.Frame);
	queue->SetField(queue_tailField->SlotIndex, context.Collector.FromValue((tail + 1) % static_cast<std::int64_t>(capacity)), context.Frame);
	queue->SetField(queue_sizeField->SlotIndex, context.Collector.FromValue(size + 1), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_queue_Dequeue(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	std::int64_t size = queue->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger();
	if (size == 0)
		throw std::runtime_error("queue is empty");

	ObjectInstance* array = queue->GetField(queue_arrayField->SlotIndex, context.Frame);
	std::int64_t head = queue->GetField(queue_headField->SlotIndex, context.Frame)->AsInteger();
	std::size_t capacity = array->GetArrayLength();

	ObjectInstance* value = array->GetElement(static_cast<std::size_t>(head), context.Frame);
	queue->SetField(queue_headField->SlotIndex, context.Collector.FromValue((head + 1) % static_cast<std::int64_t>(capacity)), context.Frame);
	queue->SetField(queue_sizeField->SlotIndex, context.Collector.FromValue(size - 1), context.Frame);
	return value;
}

static ObjectInstance* shard_queue_Peek(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	std::int64_t size = queue->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger();
	if (size == 0)
		throw std::runtime_error("queue is empty");

	ObjectInstance* array = queue->GetField(queue_arrayField->SlotIndex, context.Frame);
	std::int64_t head = queue->GetField(queue_headField->SlotIndex, context.Frame)->AsInteger();
	return array->GetElement(static_cast<std::size_t>(head), context.Frame);
}

static ObjectInstance* shard_queue_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	queue->SetField(queue_arrayField->SlotIndex, context.Collector.AllocateArray(elementType, 0), context.Frame);
	queue->SetField(queue_headField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	queue->SetField(queue_tailField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	queue->SetField(queue_sizeField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_queue_Count_get(const CallState& context) noexcept(false)
{
	return context.Collector.FromValue(context.Args[0]->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger());
}

static ObjectInstance* shard_queue_Contains(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	ObjectInstance* value = context.Args[1];

	ObjectInstance* array = queue->GetField(queue_arrayField->SlotIndex, context.Frame);
	std::int64_t size = queue->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger();
	std::int64_t head = queue->GetField(queue_headField->SlotIndex, context.Frame)->AsInteger();
	std::size_t capacity = array->GetArrayLength();

	for (std::int64_t i = 0; i < size; ++i)
	{
		std::size_t idx = static_cast<std::size_t>((head + i) % static_cast<std::int64_t>(capacity));
		if (ObjectsEqual(array->GetElement(idx, context.Frame), value))
			return context.Collector.FromValue(true);
	}

	return context.Collector.FromValue(false);
}

static ObjectInstance* shard_queue_GetEnumerator(const CallState& context) noexcept(false)
{
	ObjectInstance* queue = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(queueEnumeratorClass_raw, std::vector<TypeSymbol*>{ elementType });
	enumerator->SetField(queueEnumerator_sourceField->SlotIndex, queue, context.Frame);
	enumerator->SetField(queueEnumerator_indexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	return enumerator;
}

static ObjectInstance* shard_queueenumerator_MoveNext(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	ObjectInstance* source = self->GetField(queueEnumerator_sourceField->SlotIndex, context.Frame);
	std::int64_t index = self->GetField(queueEnumerator_indexField->SlotIndex, context.Frame)->AsInteger() + 1;
	std::int64_t size = source->GetField(queue_sizeField->SlotIndex, context.Frame)->AsInteger();

	self->SetField(queueEnumerator_indexField->SlotIndex, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(index < size);
}

static ObjectInstance* shard_queueenumerator_Current_get(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(queueEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(queueEnumerator_sourceField->SlotIndex, context.Frame);

	ObjectInstance* array = source->GetField(queue_arrayField->SlotIndex, context.Frame);
	std::int64_t head = source->GetField(queue_headField->SlotIndex, context.Frame)->AsInteger();
	std::size_t capacity = array->GetArrayLength();
	std::size_t idx = static_cast<std::size_t>((head + index) % static_cast<std::int64_t>(capacity));
	return array->GetElement(idx, context.Frame);
}

// =========================================================================
//  Stack<T> implementation
// =========================================================================
static void stack_EnsureCapacity(
	ObjectInstance* stack,
	TypeSymbol* elementType,
	CallStackFrame* frame,
	GarbageCollector& collector)
{
	ObjectInstance* array = stack->GetField(stack_arrayField->SlotIndex, frame);
	std::int64_t size = stack->GetField(stack_sizeField->SlotIndex, frame)->AsInteger();
	std::size_t capacity = array ? array->GetArrayLength() : 0;

	if (static_cast<std::size_t>(size) < capacity)
		return;

	std::size_t newCapacity = capacity == 0 ? 4 : capacity * 2;
	ObjectInstance* newArray = collector.AllocateArray(elementType, newCapacity);
	for (std::int64_t i = 0; i < size; ++i)
		newArray->SetElement(static_cast<std::size_t>(i), array->GetElement(static_cast<std::size_t>(i), frame), frame);

	stack->SetField(stack_arrayField->SlotIndex, newArray, frame);
}

static ObjectInstance* shard_stack_init(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	stack->SetField(stack_arrayField->SlotIndex, context.Collector.AllocateArray(elementType, 0), context.Frame);
	stack->SetField(stack_sizeField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_stack_Push(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	ObjectInstance* value = context.Args[1];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	stack_EnsureCapacity(stack, elementType, context.Frame, context.Collector);

	ObjectInstance* array = stack->GetField(stack_arrayField->SlotIndex, context.Frame);
	std::int64_t size = stack->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();

	array->SetElement(static_cast<std::size_t>(size), value, context.Frame);
	stack->SetField(stack_sizeField->SlotIndex, context.Collector.FromValue(size + 1), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_stack_Pop(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	std::int64_t size = stack->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();
	if (size == 0)
		throw std::runtime_error("stack is empty");

	ObjectInstance* array = stack->GetField(stack_arrayField->SlotIndex, context.Frame);
	ObjectInstance* value = array->GetElement(static_cast<std::size_t>(size - 1), context.Frame);
	stack->SetField(stack_sizeField->SlotIndex, context.Collector.FromValue(size - 1), context.Frame);
	return value;
}

static ObjectInstance* shard_stack_Peek(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	std::int64_t size = stack->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();
	if (size == 0)
		throw std::runtime_error("stack is empty");

	ObjectInstance* array = stack->GetField(stack_arrayField->SlotIndex, context.Frame);
	return array->GetElement(static_cast<std::size_t>(size - 1), context.Frame);
}

static ObjectInstance* shard_stack_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	stack->SetField(stack_arrayField->SlotIndex, context.Collector.AllocateArray(elementType, 0), context.Frame);
	stack->SetField(stack_sizeField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)), context.Frame);
	return nullptr;
}

static ObjectInstance* shard_stack_Count_get(const CallState& context) noexcept(false)
{
	return context.Collector.FromValue(context.Args[0]->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger());
}

static ObjectInstance* shard_stack_Contains(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	ObjectInstance* value = context.Args[1];

	ObjectInstance* array = stack->GetField(stack_arrayField->SlotIndex, context.Frame);
	std::int64_t size = stack->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();

	for (std::int64_t i = size - 1; i >= 0; --i)
	{
		if (ObjectsEqual(array->GetElement(static_cast<std::size_t>(i), context.Frame), value))
			return context.Collector.FromValue(true);
	}

	return context.Collector.FromValue(false);
}

static ObjectInstance* shard_stack_GetEnumerator(const CallState& context) noexcept(false)
{
	ObjectInstance* stack = context.Args[0];
	TypeSymbol* elementType = context.Frame->TypeArguments[0];

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(stackEnumeratorClass_raw, std::vector<TypeSymbol*>{ elementType });
	enumerator->SetField(stackEnumerator_sourceField->SlotIndex, stack, context.Frame);
	enumerator->SetField(stackEnumerator_indexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)), context.Frame);
	return enumerator;
}

static ObjectInstance* shard_stackenumerator_MoveNext(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	ObjectInstance* source = self->GetField(stackEnumerator_sourceField->SlotIndex, context.Frame);
	std::int64_t index = self->GetField(stackEnumerator_indexField->SlotIndex, context.Frame)->AsInteger() + 1;
	std::int64_t size = source->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();

	self->SetField(stackEnumerator_indexField->SlotIndex, context.Collector.FromValue(index), context.Frame);
	return context.Collector.FromValue(index < size);
}

static ObjectInstance* shard_stackenumerator_Current_get(const CallState& context) noexcept(false)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(stackEnumerator_indexField->SlotIndex, context.Frame)->AsInteger();
	ObjectInstance* source = self->GetField(stackEnumerator_sourceField->SlotIndex, context.Frame);

	ObjectInstance* array = source->GetField(stack_arrayField->SlotIndex, context.Frame);
	std::int64_t size = source->GetField(stack_sizeField->SlotIndex, context.Frame)->AsInteger();
	std::size_t realIndex = static_cast<std::size_t>(size - 1 - index);
	return array->GetElement(realIndex, context.Frame);
}

// =========================================================================
//  Library metadata
// =========================================================================
SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.collections";
	lib.Description = L"Standard collections";
	lib.Version = L"0.3.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> collectionsNs(context, L"collections");
	SymbolFactory factory(context.GetSemanticModel().Table.get());

	// --- class ListEnumerator<T> ---
	SymbolBuilder<StructSymbol> listEnumeratorClass = collectionsNs.AddStruct(L"ListEnumerator");

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

	TypeParameterSymbol* listClass_typeParam_T = listClass
		.AddTypeParameter(L"T");

	listClass.Implements(
		factory.GenericType(TRAIT_ENUMERABLE, { { L"T", listClass_typeParam_T } }));

	list_arrayField = listClass
		.AddField(L"_array", listClass.GetFactory().Array(listClass_typeParam_T), LINK_INSTANCE, ACS_PRIVATE).Get();

	listClass.AddInit()
		.SetCallback(&shard_list_init);

	SymbolBuilder<PropertySymbol> lengthProp = listClass.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE);
	lengthProp.AddGetter()
		.SetCallback(&shard_list_Length_get);

	SymbolBuilder<IndexatorSymbol> indexer = listClass.AddIndexer(listClass_typeParam_T, LINK_INSTANCE);
	indexer.AddParameter(L"index", TYPE_INT);
	indexer.AddGetter()
		.SetCallback(&shard_list_Indexer_get);

	indexer.AddSetter()
		.SetCallback(&shard_list_Indexer_set);

	SymbolBuilder<MethodSymbol> addMethod = listClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE);
	addMethod
		.AddParameter(L"item", listClass_typeParam_T)
		.SetCallback(&shard_list_Add);

	SymbolBuilder<MethodSymbol> elementAtMethod = listClass.AddMethod(L"ElementAt", listClass_typeParam_T, LINK_INSTANCE);
	elementAtMethod
		.AddParameter(L"index", TYPE_INT)
		.SetCallback(&shard_list_ElementAt);

	SymbolBuilder<MethodSymbol> removeAtMethod = listClass.AddMethod(L"RemoveAt", TYPE_VOID, LINK_INSTANCE);
	removeAtMethod
		.AddParameter(L"index", TYPE_INT)
		.SetCallback(&shard_list_RemoveAt);

	SymbolBuilder<MethodSymbol> clearMethod = listClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE);
	clearMethod
		.SetCallback(&shard_list_Clear);

	SymbolBuilder<MethodSymbol> getEnumeratorMethod = listClass.AddMethod(
		L"GetEnumerator",
		factory.GenericType(TRAIT_ENUMERATOR, { { L"T", listClass_typeParam_T } }),
		LINK_INSTANCE);

	getEnumeratorMethod
		.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
		.SetCallback(&shard_list_GetEnumerator);

	// --- struct KeyValuePair<K, V> ---
	SymbolBuilder<StructSymbol> kvpClass = collectionsNs.AddStruct(L"KeyValuePair");

	TypeParameterSymbol* kvp_typeParam_K = kvpClass.AddTypeParameter(L"K");
	TypeParameterSymbol* kvp_typeParam_V = kvpClass.AddTypeParameter(L"V");

	keyValuePairClass_raw = kvpClass.Get();
	keyValuePair_typeParam_K = kvp_typeParam_K;
	keyValuePair_typeParam_V = kvp_typeParam_V;

	kvp_keyField = kvpClass
		.AddField(L"Key", kvp_typeParam_K, LINK_INSTANCE, ACS_PUBLIC).Get();

	kvp_valueField = kvpClass
		.AddField(L"Value", kvp_typeParam_V, LINK_INSTANCE, ACS_PUBLIC).Get();

	// --- class Dictionary<K, V> ---
	SymbolBuilder<ClassSymbol> dictClass = collectionsNs.AddClass(L"Dictionary");

	TypeParameterSymbol* dict_typeParam_K = dictClass.AddTypeParameter(L"K");
	TypeParameterSymbol* dict_typeParam_V = dictClass.AddTypeParameter(L"V");

	dictionaryClass_raw = dictClass.Get();
	dictionary_typeParam_K = dict_typeParam_K;
	dictionary_typeParam_V = dict_typeParam_V;

	TypeSymbol* kvpGeneric = factory.GenericType(keyValuePairClass_raw,
	{ { L"K", dict_typeParam_K }, { L"V", dict_typeParam_V } });

	dictClass.Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", kvpGeneric } }));

	dict_keysField = dictClass
		.AddField(L"_keys", dictClass.GetFactory().Array(dict_typeParam_K), LINK_INSTANCE, ACS_PRIVATE).Get();

	dict_valuesField = dictClass
		.AddField(L"_values", dictClass.GetFactory().Array(dict_typeParam_V), LINK_INSTANCE, ACS_PRIVATE).Get();

	dict_hashesField = dictClass
		.AddField(L"_hashes", dictClass.GetFactory().Array(SymbolTable::Primitives::Integer), LINK_INSTANCE, ACS_PRIVATE).Get();

	dict_statesField = dictClass
		.AddField(L"_states", dictClass.GetFactory().Array(SymbolTable::Primitives::Integer), LINK_INSTANCE, ACS_PRIVATE).Get();

	dict_countField = dictClass
		.AddField(L"_count", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	dictClass.AddInit()
		.SetCallback(&shard_dict_init);

	SymbolBuilder<PropertySymbol> dictCountProp = dictClass.AddProperty(L"Count", TYPE_INT, LINK_INSTANCE);
	dictCountProp.AddGetter()
		.SetCallback(&shard_dict_Count_get);

	SymbolBuilder<PropertySymbol> dictKeysProp = dictClass.AddProperty(L"Keys", dictClass.GetFactory().Array(dict_typeParam_K), LINK_INSTANCE);
	dictKeysProp.AddGetter()
		.SetCallback(&shard_dict_Keys_get);

	SymbolBuilder<PropertySymbol> dictValuesProp = dictClass.AddProperty(L"Values", dictClass.GetFactory().Array(dict_typeParam_V), LINK_INSTANCE);
	dictValuesProp.AddGetter()
		.SetCallback(&shard_dict_Values_get);

	SymbolBuilder<IndexatorSymbol> dictIndexer = dictClass.AddIndexer(dict_typeParam_V, LINK_INSTANCE);
	dictIndexer.AddParameter(L"key", dict_typeParam_K);
	dictIndexer.AddGetter()
		.SetCallback(&shard_dict_Indexer_get);

	dictIndexer.AddSetter()
		.SetCallback(&shard_dict_Indexer_set);

	SymbolBuilder<MethodSymbol> dictAddMethod = dictClass.AddMethod(L"Add", TYPE_VOID, LINK_INSTANCE);
	dictAddMethod
		.AddParameter(L"key", dict_typeParam_K)
		.AddParameter(L"value", dict_typeParam_V)
		.SetCallback(&shard_dict_Add);

	SymbolBuilder<MethodSymbol> dictRemoveMethod = dictClass.AddMethod(L"Remove", SymbolTable::Primitives::Boolean, LINK_INSTANCE);
	dictRemoveMethod
		.AddParameter(L"key", dict_typeParam_K)
		.SetCallback(&shard_dict_Remove);

	SymbolBuilder<MethodSymbol> dictContainsMethod = dictClass.AddMethod(L"ContainsKey", SymbolTable::Primitives::Boolean, LINK_INSTANCE);
	dictContainsMethod
		.AddParameter(L"key", dict_typeParam_K)
		.SetCallback(&shard_dict_ContainsKey);

	SymbolBuilder<MethodSymbol> dictClearMethod = dictClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE);
	dictClearMethod
		.SetCallback(&shard_dict_Clear);

	SymbolBuilder<MethodSymbol> dictGetEnumeratorMethod = dictClass.AddMethod(
		L"GetEnumerator",
		factory.GenericType(TRAIT_ENUMERATOR, { { L"T", kvpGeneric } }),
		LINK_INSTANCE);

	dictGetEnumeratorMethod
		.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
		.SetCallback(&shard_dict_GetEnumerator);

	// --- struct DictionaryEnumerator<K, V> ---
	SymbolBuilder<StructSymbol> dictEnumClass = collectionsNs.AddStruct(L"DictionaryEnumerator");

	TypeParameterSymbol* dictEnum_typeParam_K = dictEnumClass.AddTypeParameter(L"K");
	TypeParameterSymbol* dictEnum_typeParam_V = dictEnumClass.AddTypeParameter(L"V");

	dictionaryEnumeratorClass_raw = dictEnumClass.Get();
	dictEnumerator_typeParam_K = dictEnum_typeParam_K;
	dictEnumerator_typeParam_V = dictEnum_typeParam_V;

	TypeSymbol* enumKvpGeneric = factory.GenericType(keyValuePairClass_raw,
	{ { L"K", dictEnum_typeParam_K }, { L"V", dictEnum_typeParam_V } });

	dictEnumClass.Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", enumKvpGeneric } }));

	dictEnumerator_sourceField = dictEnumClass
		.AddField(L"_source", factory.GenericType(dictionaryClass_raw,
		{ { L"K", dictEnum_typeParam_K }, { L"V", dictEnum_typeParam_V } }), LINK_INSTANCE, ACS_PRIVATE).Get();

	dictEnumerator_indexField = dictEnumClass
		.AddField(L"_index", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	dictEnumClass.AddMethod(L"MoveNext", SymbolTable::Primitives::Boolean, LINK_INSTANCE)
		.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
		.SetCallback(&shard_dictionaryenumerator_MoveNext);

	SymbolBuilder<PropertySymbol> dictEnumCurrentProp = dictEnumClass.AddProperty(L"Current", enumKvpGeneric, LINK_INSTANCE);
	dictEnumCurrentProp.AddGetter()
		.SetCallback(&shard_dictionaryenumerator_Current_get)
		.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET);

	// --- class Queue<T> ---
	SymbolBuilder<ClassSymbol> queueClass = collectionsNs.AddClass(L"Queue");

	TypeParameterSymbol* queueClass_typeParam_T = queueClass.AddTypeParameter(L"T");

	queueClass_raw = queueClass.Get();
	queue_typeParam_T = queueClass_typeParam_T;

	queueClass.Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", queueClass_typeParam_T } }));

	queue_arrayField = queueClass
		.AddField(L"_array", queueClass.GetFactory().Array(queueClass_typeParam_T), LINK_INSTANCE, ACS_PRIVATE).Get();

	queue_headField = queueClass
		.AddField(L"_head", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	queue_tailField = queueClass
		.AddField(L"_tail", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	queue_sizeField = queueClass
		.AddField(L"_size", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	queueClass.AddInit()
		.SetCallback(&shard_queue_init);

	SymbolBuilder<PropertySymbol> queueCountProp = queueClass.AddProperty(L"Count", TYPE_INT, LINK_INSTANCE);
	queueCountProp.AddGetter()
		.SetCallback(&shard_queue_Count_get);

	SymbolBuilder<MethodSymbol> queueEnqueueMethod = queueClass.AddMethod(L"Enqueue", TYPE_VOID, LINK_INSTANCE);
	queueEnqueueMethod
		.AddParameter(L"item", queueClass_typeParam_T)
		.SetCallback(&shard_queue_Enqueue);

	SymbolBuilder<MethodSymbol> queueDequeueMethod = queueClass.AddMethod(L"Dequeue", queueClass_typeParam_T, LINK_INSTANCE);
	queueDequeueMethod
		.SetCallback(&shard_queue_Dequeue);

	SymbolBuilder<MethodSymbol> queuePeekMethod = queueClass.AddMethod(L"Peek", queueClass_typeParam_T, LINK_INSTANCE);
	queuePeekMethod
		.SetCallback(&shard_queue_Peek);

	SymbolBuilder<MethodSymbol> queueClearMethod = queueClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE);
	queueClearMethod
		.SetCallback(&shard_queue_Clear);

	SymbolBuilder<MethodSymbol> queueContainsMethod = queueClass.AddMethod(L"Contains", SymbolTable::Primitives::Boolean, LINK_INSTANCE);
	queueContainsMethod
		.AddParameter(L"item", queueClass_typeParam_T)
		.SetCallback(&shard_queue_Contains);

	SymbolBuilder<MethodSymbol> queueGetEnumeratorMethod = queueClass.AddMethod(
		L"GetEnumerator",
		factory.GenericType(TRAIT_ENUMERATOR, { { L"T", queueClass_typeParam_T } }),
		LINK_INSTANCE);

	queueGetEnumeratorMethod
		.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
		.SetCallback(&shard_queue_GetEnumerator);

	// --- struct QueueEnumerator<T> ---
	SymbolBuilder<StructSymbol> queueEnumClass = collectionsNs.AddStruct(L"QueueEnumerator");

	TypeParameterSymbol* queueEnum_typeParam_T = queueEnumClass.AddTypeParameter(L"T");

	queueEnumeratorClass_raw = queueEnumClass.Get();
	queueEnumerator_typeParam_T = queueEnum_typeParam_T;

	queueEnumClass.Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", queueEnum_typeParam_T } }));

	queueEnumerator_sourceField = queueEnumClass
		.AddField(L"_source", factory.GenericType(queueClass_raw, { { L"T", queueEnum_typeParam_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

	queueEnumerator_indexField = queueEnumClass
		.AddField(L"_index", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	queueEnumClass.AddMethod(L"MoveNext", SymbolTable::Primitives::Boolean, LINK_INSTANCE)
		.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
		.SetCallback(&shard_queueenumerator_MoveNext);

	SymbolBuilder<PropertySymbol> queueEnumCurrentProp = queueEnumClass.AddProperty(L"Current", queueEnum_typeParam_T, LINK_INSTANCE);
	queueEnumCurrentProp.AddGetter()
		.SetCallback(&shard_queueenumerator_Current_get)
		.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET);

	// --- class Stack<T> ---
	SymbolBuilder<ClassSymbol> stackClass = collectionsNs.AddClass(L"Stack");

	TypeParameterSymbol* stackClass_typeParam_T = stackClass.AddTypeParameter(L"T");

	stackClass_raw = stackClass.Get();
	stack_typeParam_T = stackClass_typeParam_T;

	stackClass.Implements(factory.GenericType(TRAIT_ENUMERABLE, { { L"T", stackClass_typeParam_T } }));

	stack_arrayField = stackClass
		.AddField(L"_array", stackClass.GetFactory().Array(stackClass_typeParam_T), LINK_INSTANCE, ACS_PRIVATE).Get();

	stack_sizeField = stackClass
		.AddField(L"_size", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	stackClass.AddInit()
		.SetCallback(&shard_stack_init);

	SymbolBuilder<PropertySymbol> stackCountProp = stackClass.AddProperty(L"Count", TYPE_INT, LINK_INSTANCE);
	stackCountProp.AddGetter()
		.SetCallback(&shard_stack_Count_get);

	SymbolBuilder<MethodSymbol> stackPushMethod = stackClass.AddMethod(L"Push", TYPE_VOID, LINK_INSTANCE);
	stackPushMethod
		.AddParameter(L"item", stackClass_typeParam_T)
		.SetCallback(&shard_stack_Push);

	SymbolBuilder<MethodSymbol> stackPopMethod = stackClass.AddMethod(L"Pop", stackClass_typeParam_T, LINK_INSTANCE);
	stackPopMethod
		.SetCallback(&shard_stack_Pop);

	SymbolBuilder<MethodSymbol> stackPeekMethod = stackClass.AddMethod(L"Peek", stackClass_typeParam_T, LINK_INSTANCE);
	stackPeekMethod
		.SetCallback(&shard_stack_Peek);

	SymbolBuilder<MethodSymbol> stackClearMethod = stackClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE);
	stackClearMethod
		.SetCallback(&shard_stack_Clear);

	SymbolBuilder<MethodSymbol> stackContainsMethod = stackClass.AddMethod(L"Contains", SymbolTable::Primitives::Boolean, LINK_INSTANCE);
	stackContainsMethod
		.AddParameter(L"item", stackClass_typeParam_T)
		.SetCallback(&shard_stack_Contains);

	SymbolBuilder<MethodSymbol> stackGetEnumeratorMethod = stackClass.AddMethod(
		L"GetEnumerator",
		factory.GenericType(TRAIT_ENUMERATOR, { { L"T", stackClass_typeParam_T } }),
		LINK_INSTANCE);

	stackGetEnumeratorMethod
		.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
		.SetCallback(&shard_stack_GetEnumerator);

	// --- struct StackEnumerator<T> ---
	SymbolBuilder<StructSymbol> stackEnumClass = collectionsNs.AddStruct(L"StackEnumerator");

	TypeParameterSymbol* stackEnum_typeParam_T = stackEnumClass.AddTypeParameter(L"T");

	stackEnumeratorClass_raw = stackEnumClass.Get();
	stackEnumerator_typeParam_T = stackEnum_typeParam_T;

	stackEnumClass.Implements(factory.GenericType(TRAIT_ENUMERATOR, { { L"T", stackEnum_typeParam_T } }));

	stackEnumerator_sourceField = stackEnumClass
		.AddField(L"_source", factory.GenericType(stackClass_raw, { { L"T", stackEnum_typeParam_T } }), LINK_INSTANCE, ACS_PRIVATE).Get();

	stackEnumerator_indexField = stackEnumClass
		.AddField(L"_index", SymbolTable::Primitives::Integer, LINK_INSTANCE, ACS_PRIVATE).Get();

	stackEnumClass.AddMethod(L"MoveNext", SymbolTable::Primitives::Boolean, LINK_INSTANCE)
		.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
		.SetCallback(&shard_stackenumerator_MoveNext);

	SymbolBuilder<PropertySymbol> stackEnumCurrentProp = stackEnumClass.AddProperty(L"Current", stackEnum_typeParam_T, LINK_INSTANCE);
	stackEnumCurrentProp.AddGetter()
		.SetCallback(&shard_stackenumerator_Current_get)
		.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET);
}
