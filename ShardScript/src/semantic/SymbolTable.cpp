#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

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
	arrayLengthGetter->set_calling_convention(CallingConvention::LibFunc);
	arrayLengthGetter->set_effects_computed(true);

	arrayLength->set_getter(arrayLengthGetter);
	m_array->properties.push_back(arrayLength);
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

namespace
{
	// word-key layout: family tag, base type offset, then argument offsets
	// (generics) or length (sized arrays); the interner copies keys into its
	// own chunk storage, so stack-built keys are safe to pass
	constexpr std::uint64_t ArrayFamilyTag = static_cast<std::uint64_t>(SyntaxKind::ArrayType);
	constexpr std::uint64_t GenericFamilyTag = static_cast<std::uint64_t>(SyntaxKind::GenericType);
	constexpr std::uint64_t NullableFamilyTag = static_cast<std::uint64_t>(SyntaxKind::NullableType);
	constexpr std::size_t InlineKeyWords = 8;

	std::uint64_t* make_generic_key(std::uint64_t* inlineKey, std::pmr::vector<std::uint64_t>& spilledKey, gmt::Ref<StructSymbol> underlaying, std::span<const gmt::Ref<StructSymbol>> typeArguments)
	{
		const std::size_t count = 2 + typeArguments.size();
		std::uint64_t* key = inlineKey;

		if (count > InlineKeyWords)
		{
			spilledKey.resize(count);
			key = spilledKey.data();
		}

		key[0] = GenericFamilyTag;
		key[1] = underlaying.get_offset();
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
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), underlaying, typeArguments);

	gmt::Ref<SyntaxSymbol> symbol = m_interner.find(key, static_cast<std::uint16_t>(2 + typeArguments.size()));
	if (symbol.is_null() || symbol->get_kind() != SyntaxKind::GenericType)
		return gmt::nullref;

	return symbol.as<GenericTypeSymbol>();
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
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), underlaying, typeArguments);

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
