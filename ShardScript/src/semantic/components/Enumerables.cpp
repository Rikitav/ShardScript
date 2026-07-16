#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>

#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <cstdint>

using namespace shard;

namespace
{
	static inline void inherit_size(FieldSymbol* field)
	{
		TypeSymbol* parent = static_cast<TypeSymbol*>(field->Parent);
		if (field->SlotIndex == std::numeric_limits<std::uint32_t>::max())
			field->SlotIndex = parent->NextSlotIndex++;

		field->MemoryBytesOffset = parent->MemoryBytesSize;
		parent->MemoryBytesSize += field->ReturnType->GetInlineSize();
	}
}

static ObjectInstance* array_enumerator_MoveNext(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex)->AsInteger();
	std::int64_t length = self->GetField(CLASS_ARRAYENUMERATOR_LengthField->SlotIndex)->AsInteger();

	index++;
	self->SetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex, context.Collector.FromValue(index));
	return context.Collector.FromValue(index < length);
}

static ObjectInstance* array_enumerator_Current_get(const CallState& context)
{
	ObjectInstance* self = context.Args[0];
	std::int64_t index = self->GetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex)->AsInteger();
	ObjectInstance* source = self->GetField(CLASS_ARRAYENUMERATOR_SourceField->SlotIndex);
	return source->GetElement(static_cast<std::size_t>(index));
}

static ObjectInstance* primitive_array_get_enumerator(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(array->getInfo());
	TypeSymbol* concreteT = const_cast<TypeSymbol*>(arrayType->UnderlayingType);

	ObjectInstance* enumerator = context.Collector.AllocateGeneric(CLASS_ARRAYENUMERATOR, std::vector<TypeSymbol*>{ concreteT });
	enumerator->SetField(CLASS_ARRAYENUMERATOR_SourceField->SlotIndex, array);
	enumerator->SetField(CLASS_ARRAYENUMERATOR_IndexField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(-1)));
	enumerator->SetField(CLASS_ARRAYENUMERATOR_LengthField->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(arrayType->Length)));

	return enumerator;
}

static ObjectInstance* primitive_array_Length_get(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	return context.Collector.FromValue(static_cast<std::int64_t>(array->GetArrayLength()));
}

static ObjectInstance* primitive_array_GetElement(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();
	return array->GetElement(static_cast<std::size_t>(index), context.Frame);
}

static ObjectInstance* primitive_array_SetElement(const CallState& context)
{
	ObjectInstance* array = context.Args[0];
	std::int64_t index = context.Args[1]->AsInteger();
	ObjectInstance* value = context.Args[2];
	array->SetElement(static_cast<std::size_t>(index), value, context.Frame);
	return nullptr;
}

void SymbolTable::ResolveEnumerables(SymbolTable* globalTable)
{
	SymbolFactory factory(globalTable);

	// ArrayEnumerator<T>
	{
		SymbolBuilder<ClassSymbol> builder(globalTable, L"ArrayEnumerator", SymbolTable::Global::Namespace);
		CLASS_ARRAYENUMERATOR = builder
			.Implements(TRAIT_ENUMERATOR)
			.DeclareGlobal();

		CLASS_ARRAYENUMERATOR_T = builder
			.AddTypeParameter(L"T");

		CLASS_ARRAYENUMERATOR_SourceField = builder
			.AddField(L"_source", factory.Array(CLASS_ARRAYENUMERATOR_T), LINK_INSTANCE, ACS_PRIVATE);

		CLASS_ARRAYENUMERATOR_IndexField = builder
			.AddField(L"_index", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

		CLASS_ARRAYENUMERATOR_LengthField = builder
			.AddField(L"_length", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

		inherit_size(CLASS_ARRAYENUMERATOR_SourceField);
		inherit_size(CLASS_ARRAYENUMERATOR_IndexField);
		inherit_size(CLASS_ARRAYENUMERATOR_LengthField);

		builder.AddMethod(L"MoveNext", TYPE_BOOL, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_ENUMERATOR_MOVENEXT)
			.SetCallback(&array_enumerator_MoveNext);

		builder.AddProperty(L"Current", CLASS_ARRAYENUMERATOR_T, LINK_INSTANCE)
			.AddGetter()
			.IsImplementationOf(TRAIT_ENUMERATOR_CURRENT_GET)
			.SetCallback(&array_enumerator_Current_get);
	}

	// Make the primitive array type implement IEnumerable<T> through a native GetEnumerator
	{
		SymbolBuilder<ClassSymbol> builder(globalTable, static_cast<ClassSymbol*>(TYPE_ARRAY));
		builder.Implements(TRAIT_ENUMERABLE);

		GenericTypeSymbol* enumeratorReturnType = factory.GenericType(TRAIT_ENUMERATOR, { { L"T", TRAIT_ENUMERATOR->TypeParameters[0] } });
		builder.AddMethod(L"GetEnumerator", enumeratorReturnType, LINK_INSTANCE)
			.IsImplementationOf(TRAIT_ENUMERABLE_GETENUMERATOR)
			.SetCallback(&primitive_array_get_enumerator);

		builder.AddProperty(L"Length", TYPE_INT, LINK_INSTANCE)
			.AddGetter()
			.SetCallback(&primitive_array_Length_get);

		SymbolBuilder<IndexatorSymbol> arrayIndexer = std::move(builder.AddIndexer(TYPE_ANY, LINK_INSTANCE)
			.AddParameter(L"index", TYPE_INT));

		arrayIndexer.AddGetter()
			.SetCallback(&primitive_array_GetElement);

		arrayIndexer.AddSetter()
			.SetCallback(&primitive_array_SetElement);
	}
}