#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/InlineVector.hpp>

using namespace shard;

SymbolTable::SymbolTable()
	: m_rootNamespace(m_arena.emplace<NamespaceSymbol>(L"", gmt::nullref))
{
	m_void = m_arena.emplace<StructSymbol>(L"void", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_boolean = m_arena.emplace<StructSymbol>(L"bool", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_integer = m_arena.emplace<StructSymbol>(L"int", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_double = m_arena.emplace<StructSymbol>(L"double", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_char = m_arena.emplace<StructSymbol>(L"char", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_string = m_arena.emplace<StructSymbol>(L"string", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_array = m_arena.emplace<StructSymbol>(L"Array", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_nativeInteger = m_arena.emplace<StructSymbol>(L"nint", SyntaxKind::StructDeclaration, m_rootNamespace);
	m_byte = m_arena.emplace<StructSymbol>(L"byte", SyntaxKind::StructDeclaration, m_rootNamespace);

	// the runtime wires the getter's function pointer when a virtual machine exists
	gmt::Ref<PropertySymbol> arrayLength = m_arena.emplace<PropertySymbol>(L"Length", SyntaxKind::PropertyDeclaration, m_array);
	arrayLength->set_return_type(m_integer);
	arrayLength->set_accesibility(ACS_PUBLIC);
	arrayLength->set_linking(LINK_INSTANCE);

	gmt::Ref<AccessorSymbol> arrayLengthGetter = m_arena.emplace<AccessorSymbol>(L"get", arrayLength);
	arrayLengthGetter->set_return_type(m_integer);
	arrayLengthGetter->set_linking(LINK_INSTANCE);
	arrayLengthGetter->set_accesibility(ACS_PUBLIC);
	arrayLengthGetter->set_calling_convention(CallingConvention::ShardDecl);
	arrayLengthGetter->set_effects_computed(true);

	arrayLength->set_getter(arrayLengthGetter);
	m_array->properties.push_back(arrayLength);

	// lazy integer range sequence
	gmt::Ref<StructSymbol> rangeEnumerable = m_arena.emplace<StructSymbol>(L"RangeEnumerable", SyntaxKind::StructDeclaration, m_rootNamespace);

	gmt::Ref<FieldSymbol> rangeFromField = m_arena.emplace<FieldSymbol>(L"_from", rangeEnumerable);
	rangeFromField->set_return_type(m_integer);
	rangeFromField->set_accesibility(ACS_PRIVATE);
	rangeFromField->set_linking(LINK_INSTANCE);

	gmt::Ref<FieldSymbol> rangeToField = m_arena.emplace<FieldSymbol>(L"_to", rangeEnumerable);
	rangeToField->set_return_type(m_integer);
	rangeToField->set_accesibility(ACS_PRIVATE);
	rangeToField->set_linking(LINK_INSTANCE);

	gmt::Ref<FieldSymbol> rangeCurrentField = m_arena.emplace<FieldSymbol>(L"_current", rangeEnumerable);
	rangeCurrentField->set_return_type(m_integer);
	rangeCurrentField->set_accesibility(ACS_PRIVATE);
	rangeCurrentField->set_linking(LINK_INSTANCE);

	gmt::Ref<MethodSymbol> rangeMoveNext = m_arena.emplace<MethodSymbol>(L"MoveNext", rangeEnumerable);
	rangeMoveNext->set_return_type(m_boolean);
	rangeMoveNext->set_accesibility(ACS_PUBLIC);
	rangeMoveNext->set_linking(LINK_INSTANCE);
	rangeMoveNext->set_calling_convention(CallingConvention::ShardDecl);
	rangeMoveNext->set_effects_computed(true);

	gmt::Ref<PropertySymbol> rangeCurrent = m_arena.emplace<PropertySymbol>(L"Current", SyntaxKind::PropertyDeclaration, rangeEnumerable);
	rangeCurrent->set_return_type(m_integer);
	rangeCurrent->set_accesibility(ACS_PUBLIC);
	rangeCurrent->set_linking(LINK_INSTANCE);

	gmt::Ref<AccessorSymbol> rangeCurrentGetter = m_arena.emplace<AccessorSymbol>(L"get", rangeCurrent);
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
	gmt::Ref<InterfaceSymbol> indexable = m_arena.emplace<InterfaceSymbol>(L"IIndexable", m_rootNamespace);

	gmt::Ref<TypeParameterSymbol> indexableT = m_arena.emplace<TypeParameterSymbol>(L"T", indexable);
	indexable->type_parameters.push_back(indexableT);

	gmt::Ref<PropertySymbol> indexableCount = m_arena.emplace<PropertySymbol>(L"Count", SyntaxKind::PropertyDeclaration, indexable);
	indexableCount->set_return_type(m_integer);
	indexableCount->set_accesibility(ACS_PUBLIC);
	indexableCount->set_linking(LINK_INSTANCE);

	// accessor names follow the implementation convention ('get_Count' for
	// properties, 'get' for indexators): interface dispatch resolves by name
	gmt::Ref<AccessorSymbol> indexableCountGetter = m_arena.emplace<AccessorSymbol>(L"get_Count", indexableCount);
	indexableCountGetter->set_return_type(m_integer);
	indexableCountGetter->set_linking(LINK_INSTANCE);
	indexableCountGetter->set_accesibility(ACS_PUBLIC);
	indexableCountGetter->set_is_virtual(true);

	indexableCount->set_getter(indexableCountGetter);

	gmt::Ref<IndexatorSymbol> indexableIndexator = m_arena.emplace<IndexatorSymbol>(L"index", indexable);
	indexableIndexator->set_return_type(indexableT);
	indexableIndexator->set_accesibility(ACS_PUBLIC);
	indexableIndexator->set_linking(LINK_INSTANCE);

	gmt::Ref<ParameterSymbol> indexableIndexParam = m_arena.emplace<ParameterSymbol>(L"index", indexableIndexator);
	indexableIndexParam->set_type(m_integer);
	indexableIndexator->on_symbol_declared(indexableIndexParam);

	gmt::Ref<AccessorSymbol> indexableGetAccessor = m_arena.emplace<AccessorSymbol>(L"get", indexableIndexator);
	indexableGetAccessor->set_return_type(indexableT);
	indexableGetAccessor->set_linking(LINK_INSTANCE);
	indexableGetAccessor->set_accesibility(ACS_PUBLIC);
	indexableGetAccessor->set_is_virtual(true);

	// indexator accessors carry their own copy of the index parameters,
	// mirroring SymbolBuilder<IndexatorSymbol>::add_getter
	gmt::Ref<ParameterSymbol> indexableGetParam = m_arena.emplace<ParameterSymbol>(L"index", indexableGetAccessor);
	indexableGetParam->set_type(m_integer);
	indexableGetAccessor->on_symbol_declared(indexableGetParam);

	indexableIndexator->set_getter(indexableGetAccessor);

	indexable->properties.push_back(indexableCount);
	indexable->indexators.push_back(indexableIndexator);

	m_rootNamespace->members.push_back(indexable);
	set_standard_type(StandardTypes::IIndexable, indexable);
}

gmt::Arena& SymbolTable::get_arena()
{
	return m_arena;
}

gmt::Ref<NamespaceSymbol> SymbolTable::get_root_namespace() const
{
	return m_rootNamespace;
}

gmt::Ref<SyntaxSymbol> SymbolTable::lookup_symbol(gmt::Ref<const SyntaxNode> node)
{
	auto choice = m_nodeToSymbolMap.find(node);
	if (choice == m_nodeToSymbolMap.end())
		return gmt::nullref; // no symbol bound to this node

	return choice->second;
}

gmt::Ref<const SyntaxNode> SymbolTable::lookup_node(gmt::Ref<SyntaxSymbol> symbol)
{
	auto choice = m_symbolToNodeMap.find(symbol);
	if (choice == m_symbolToNodeMap.end())
		return gmt::nullref; // no symbol bound to this node

	return choice->second;
}

void SymbolTable::bind_type_arguments(gmt::Ref<const SyntaxNode> node, std::span<const gmt::Ref<StructSymbol>> typeArguments)
{
	m_nodeTypeArguments[node].assign(typeArguments.begin(), typeArguments.end());
}

std::span<const gmt::Ref<StructSymbol>> SymbolTable::lookup_type_arguments(gmt::Ref<const SyntaxNode> node) const
{
	auto choice = m_nodeTypeArguments.find(node);
	if (choice == m_nodeTypeArguments.end())
		return {};

	return choice->second;
}

namespace
{
	// word-key layout: family tag, base symbol offset, then argument offsets
	// (generics) or length (sized arrays); the interner copies keys into its
	// own chunk storage, so stack-built keys are safe to pass
	constexpr std::uint64_t ArrayFamilyTag = static_cast<std::uint64_t>(SyntaxKind::ArrayType);
	constexpr std::uint64_t GenericFamilyTag = static_cast<std::uint64_t>(SyntaxKind::GenericType);
	constexpr std::uint64_t NullableFamilyTag = static_cast<std::uint64_t>(SyntaxKind::NullableType);
	constexpr std::uint64_t GenericMethodFamilyTag = ~static_cast<std::uint64_t>(0);
	constexpr std::size_t InlineKeyWords = 8;

	std::uint64_t* make_generic_key(std::uint64_t* inlineKey, std::pmr::vector<std::uint64_t>& spilledKey, std::uint64_t familyTag, std::size_t underlayingOffset, std::span<const gmt::Ref<StructSymbol>> typeArguments)
	{
		const std::size_t count = 2 + typeArguments.size();
		std::uint64_t* key = inlineKey;

		if (count > InlineKeyWords)
		{
			spilledKey.resize(count);
			key = spilledKey.data();
		}

		key[0] = familyTag;
		key[1] = underlayingOffset;
		for (std::size_t i = 0; i < typeArguments.size(); ++i)
			key[2 + i] = typeArguments[i].get_offset();

		return key;
	}
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_array_type(gmt::Ref<StructSymbol> underlaying) const
{
	return find_array_type(underlaying, 0);
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size) const
{
	const std::uint64_t key[3] = { ArrayFamilyTag, underlaying.get_offset(), size };
	gmt::Ref<SyntaxSymbol> symbol = m_interner.find(key, 3);

	if (symbol.is_null() || symbol->get_kind() != SyntaxKind::ArrayType)
		return gmt::nullref;

	return symbol.as<ArrayTypeSymbol>();
}

gmt::Ref<GenericTypeSymbol> SymbolTable::find_generic_type(gmt::Ref<StructSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments) const
{
	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericFamilyTag, underlaying.get_offset(), typeArguments);

	gmt::Ref<SyntaxSymbol> symbol = m_interner.find(key, static_cast<std::uint16_t>(2 + typeArguments.size()));
	if (symbol.is_null() || symbol->get_kind() != SyntaxKind::GenericType)
		return gmt::nullref;

	return symbol.as<GenericTypeSymbol>();
}

gmt::Ref<GenericMethodSymbol> SymbolTable::find_generic_method(gmt::Ref<MethodSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments) const
{
	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericMethodFamilyTag, underlaying.get_offset(), typeArguments);

	gmt::Ref<SyntaxSymbol> symbol = m_interner.find(key, static_cast<std::uint16_t>(2 + typeArguments.size()));
	if (symbol.is_null())
		return gmt::nullref;

	return symbol.as<GenericMethodSymbol>();
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_or_create_array_type(gmt::Ref<StructSymbol> underlaying)
{
	return find_or_create_array_type(underlaying, 0);
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_or_create_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size)
{
	gmt::Ref<ArrayTypeSymbol> existing = find_array_type(underlaying, size);
	if (!existing.is_null())
		return existing;

	gmt::Ref<ArrayTypeSymbol> symbol = m_arena.emplace<ArrayTypeSymbol>(underlaying, m_rootNamespace);
	symbol->set_length(size);

	const std::uint64_t key[3] = { ArrayFamilyTag, underlaying.get_offset(), size };
	m_interner.insert(key, 3, symbol);
	return symbol;
}

gmt::Ref<GenericTypeSymbol> SymbolTable::find_or_create_generic_type(gmt::Ref<StructSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments)
{
	gmt::Ref<GenericTypeSymbol> existing = find_generic_type(underlaying, typeArguments);
	if (!existing.is_null())
		return existing;

	gmt::Ref<GenericTypeSymbol> symbol = m_arena.emplace<GenericTypeSymbol>(underlaying, m_rootNamespace);

	const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = underlaying->type_parameters;
	for (std::size_t i = 0; i < typeParameters.size() && i < typeArguments.size(); ++i)
		symbol->add_type_parameter(typeParameters[i], typeArguments[i]);

	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericFamilyTag, underlaying.get_offset(), typeArguments);

	m_interner.insert(key, static_cast<std::uint16_t>(2 + typeArguments.size()), symbol);
	return symbol;
}

gmt::Ref<GenericMethodSymbol> SymbolTable::find_or_create_generic_method(gmt::Ref<MethodSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments, gmt::Ref<StructSymbol> returnType)
{
	gmt::Ref<GenericMethodSymbol> existing = find_generic_method(underlaying, typeArguments);
	if (!existing.is_null())
		return existing;

	gmt::Ref<const SyntaxSymbol> parent = underlaying->get_parent();
	gmt::Ref<GenericMethodSymbol> symbol = m_arena.emplace<GenericMethodSymbol>(underlaying, gmt::Ref<SyntaxSymbol>(m_arena, parent.get_offset()));
	symbol->set_type_arguments(typeArguments);
	symbol->set_return_type(returnType);
	symbol->set_is_virtual(underlaying->get_is_virtual());
	symbol->set_is_async(underlaying->get_is_async());
	symbol->set_linking(underlaying->get_linking());
	symbol->set_accesibility(underlaying->get_accesibility());

	// parameter symbols stay shared with the open method: the call protocol
	// only needs their count and order, types substitute per instantiation
	for (gmt::Ref<ParameterSymbol> parameter : underlaying->get_parameters())
		symbol->on_symbol_declared(parameter);

	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericMethodFamilyTag, underlaying.get_offset(), typeArguments);

	m_interner.insert(key, static_cast<std::uint16_t>(2 + typeArguments.size()), symbol);
	return symbol;
}

gmt::Ref<NullableTypeSymbol> SymbolTable::find_nullable_type(gmt::Ref<StructSymbol> underlaying) const
{
	const std::uint64_t key[2] = { NullableFamilyTag, underlaying.get_offset() };
	gmt::Ref<SyntaxSymbol> symbol = m_interner.find(key, 2);

	if (symbol.is_null() || symbol->get_kind() != SyntaxKind::NullableType)
		return gmt::nullref;

	return symbol.as<NullableTypeSymbol>();
}

gmt::Ref<NullableTypeSymbol> SymbolTable::find_or_create_nullable_type(gmt::Ref<StructSymbol> underlaying)
{
	gmt::Ref<NullableTypeSymbol> existing = find_nullable_type(underlaying);
	if (!existing.is_null())
		return existing;

	gmt::Ref<NullableTypeSymbol> symbol = m_arena.emplace<NullableTypeSymbol>(underlaying, m_rootNamespace);

	const std::uint64_t key[2] = { NullableFamilyTag, underlaying.get_offset() };
	m_interner.insert(key, 2, symbol);
	return symbol;
}

gmt::Ref<StructSymbol> SymbolTable::get_void_primitive() const
{
	return m_void;
}

gmt::Ref<StructSymbol> SymbolTable::get_boolean_primitive() const
{
	return m_boolean;
}

gmt::Ref<StructSymbol> SymbolTable::get_integer_primitive() const
{
	return m_integer;
}

gmt::Ref<StructSymbol> SymbolTable::get_double_primitive() const
{
	return m_double;
}

gmt::Ref<StructSymbol> SymbolTable::get_char_primitive() const
{
	return m_char;
}

gmt::Ref<StructSymbol> SymbolTable::get_string_primitive() const
{
	return m_string;
}

gmt::Ref<StructSymbol> SymbolTable::get_array_primitive() const
{
	return m_array;
}

gmt::Ref<StructSymbol> SymbolTable::get_nint_primitive() const
{
	return m_nativeInteger;
}

gmt::Ref<StructSymbol> SymbolTable::get_byte_primitive() const
{
	return m_byte;
}

gmt::Ref<SyntaxSymbol> SymbolTable::get_standard_type(StandardTypes type)
{
	gmt::Ref<SyntaxSymbol>& slot = m_standardTypes[static_cast<std::size_t>(type)];

	if (slot.is_null())
		slot = m_arena.emplace<StructSymbol>(L"", SyntaxKind::StructDeclaration, m_rootNamespace);

	return slot;
}

void SymbolTable::set_standard_type(StandardTypes type, gmt::Ref<SyntaxSymbol> symbol)
{
	m_standardTypes[static_cast<std::size_t>(type)] = symbol;
}
