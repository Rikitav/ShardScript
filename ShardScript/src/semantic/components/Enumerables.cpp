#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

void SymbolTable::resolve_enumerables()
{
	// the runtime wires the getter's function pointer when a virtual machine exists
	PropertySymbol* arrayLength = m_arena.emplace<PropertySymbol>(L"Length", SyntaxKind::PropertyDeclaration, m_array);
	arrayLength->set_return_type(m_integer);
	arrayLength->set_accesibility(ACS_PUBLIC);
	arrayLength->set_linking(LINK_INSTANCE);

	AccessorSymbol* arrayLengthGetter = m_arena.emplace<AccessorSymbol>(L"get", arrayLength);
	arrayLengthGetter->set_return_type(m_integer);
	arrayLengthGetter->set_linking(LINK_INSTANCE);
	arrayLengthGetter->set_accesibility(ACS_PUBLIC);
	arrayLengthGetter->set_calling_convention(CallingConvention::ShardDecl);
	arrayLengthGetter->set_effects_computed(true);

	arrayLength->set_getter(arrayLengthGetter);
	m_array->properties.push_back(arrayLength);

	// lazy integer range sequence
	StructSymbol* rangeEnumerable = m_arena.emplace<StructSymbol>(L"RangeEnumerable", SyntaxKind::StructDeclaration, m_rootNamespace);

	FieldSymbol* rangeFromField = m_arena.emplace<FieldSymbol>(L"_from", rangeEnumerable);
	rangeFromField->set_return_type(m_integer);
	rangeFromField->set_accesibility(ACS_PRIVATE);
	rangeFromField->set_linking(LINK_INSTANCE);

	FieldSymbol* rangeToField = m_arena.emplace<FieldSymbol>(L"_to", rangeEnumerable);
	rangeToField->set_return_type(m_integer);
	rangeToField->set_accesibility(ACS_PRIVATE);
	rangeToField->set_linking(LINK_INSTANCE);

	FieldSymbol* rangeCurrentField = m_arena.emplace<FieldSymbol>(L"_current", rangeEnumerable);
	rangeCurrentField->set_return_type(m_integer);
	rangeCurrentField->set_accesibility(ACS_PRIVATE);
	rangeCurrentField->set_linking(LINK_INSTANCE);

	MethodSymbol* rangeMoveNext = m_arena.emplace<MethodSymbol>(L"MoveNext", rangeEnumerable);
	rangeMoveNext->set_return_type(m_boolean);
	rangeMoveNext->set_accesibility(ACS_PUBLIC);
	rangeMoveNext->set_linking(LINK_INSTANCE);
	rangeMoveNext->set_calling_convention(CallingConvention::ShardDecl);
	rangeMoveNext->set_effects_computed(true);

	PropertySymbol* rangeCurrent = m_arena.emplace<PropertySymbol>(L"Current", SyntaxKind::PropertyDeclaration, rangeEnumerable);
	rangeCurrent->set_return_type(m_integer);
	rangeCurrent->set_accesibility(ACS_PUBLIC);
	rangeCurrent->set_linking(LINK_INSTANCE);

	AccessorSymbol* rangeCurrentGetter = m_arena.emplace<AccessorSymbol>(L"get", rangeCurrent);
	rangeCurrentGetter->set_return_type(m_integer);
	rangeCurrentGetter->set_linking(LINK_INSTANCE);
	rangeCurrentGetter->set_accesibility(ACS_PUBLIC);
	rangeCurrentGetter->set_calling_convention(CallingConvention::ShardDecl);
	rangeCurrentGetter->set_effects_computed(true);

	rangeCurrent->set_getter(rangeCurrentGetter);

	rangeEnumerable->fields.push_back(rangeFromField);
	rangeEnumerable->fields.push_back(rangeToField);
	rangeEnumerable->fields.push_back(rangeCurrentField);
	rangeEnumerable->methods.push_back(rangeMoveNext);
	rangeEnumerable->properties.push_back(rangeCurrent);

	set_standard_type(StandardTypes::RangeEnumerable, rangeEnumerable);
	set_standard_type(StandardTypes::RangeEnumerable_FromField, rangeFromField);
	set_standard_type(StandardTypes::RangeEnumerable_ToField, rangeToField);
	set_standard_type(StandardTypes::RangeEnumerable_CurrentField, rangeCurrentField);
	set_standard_type(StandardTypes::RangeEnumerable_MoveNext, rangeMoveNext);
	set_standard_type(StandardTypes::RangeEnumerable_Current_get, rangeCurrentGetter);

	// the for-in iteration contract for index-based collections
	InterfaceSymbol* indexable = m_arena.emplace<InterfaceSymbol>(L"IIndexable", m_rootNamespace);

	TypeParameterSymbol* indexableT = m_arena.emplace<TypeParameterSymbol>(L"T", indexable);
	indexable->type_parameters.push_back(indexableT);

	PropertySymbol* indexableCount = m_arena.emplace<PropertySymbol>(L"Count", SyntaxKind::PropertyDeclaration, indexable);
	indexableCount->set_return_type(m_integer);
	indexableCount->set_accesibility(ACS_PUBLIC);
	indexableCount->set_linking(LINK_INSTANCE);

	// accessor names follow the implementation convention ('get_Count' for
	// properties, 'get' for indexators): interface dispatch resolves by name
	AccessorSymbol* indexableCountGetter = m_arena.emplace<AccessorSymbol>(L"get_Count", indexableCount);
	indexableCountGetter->set_return_type(m_integer);
	indexableCountGetter->set_linking(LINK_INSTANCE);
	indexableCountGetter->set_accesibility(ACS_PUBLIC);
	indexableCountGetter->set_is_virtual(true);

	indexableCount->set_getter(indexableCountGetter);

	IndexatorSymbol* indexableIndexator = m_arena.emplace<IndexatorSymbol>(L"index", indexable);
	indexableIndexator->set_return_type(indexableT);
	indexableIndexator->set_accesibility(ACS_PUBLIC);
	indexableIndexator->set_linking(LINK_INSTANCE);

	ParameterSymbol* indexableIndexParam = m_arena.emplace<ParameterSymbol>(L"index", indexableIndexator);
	indexableIndexParam->set_type(m_integer);
	indexableIndexator->on_symbol_declared(indexableIndexParam);

	AccessorSymbol* indexableGetAccessor = m_arena.emplace<AccessorSymbol>(L"get", indexableIndexator);
	indexableGetAccessor->set_return_type(indexableT);
	indexableGetAccessor->set_linking(LINK_INSTANCE);
	indexableGetAccessor->set_accesibility(ACS_PUBLIC);
	indexableGetAccessor->set_is_virtual(true);

	// indexator accessors carry their own copy of the index parameters,
	// mirroring SymbolBuilder<IndexatorSymbol>::add_getter
	ParameterSymbol* indexableGetParam = m_arena.emplace<ParameterSymbol>(L"index", indexableGetAccessor);
	indexableGetParam->set_type(m_integer);
	indexableGetAccessor->on_symbol_declared(indexableGetParam);

	indexableIndexator->set_getter(indexableGetAccessor);

	indexable->properties.push_back(indexableCount);
	indexable->indexators.push_back(indexableIndexator);

	m_rootNamespace->members.push_back(indexable);
	set_standard_type(StandardTypes::IIndexable, indexable);
}