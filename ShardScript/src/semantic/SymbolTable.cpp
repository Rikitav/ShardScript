#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/ParameterSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

using namespace shard;

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

	std::uint64_t* make_generic_key(std::uint64_t* inlineKey, std::pmr::vector<std::uint64_t>& spilledKey, std::uint64_t familyTag, std::size_t underlayingOffset, std::span<StructSymbol* const> typeArguments)
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
			key[2 + i] = reinterpret_cast<std::uint64_t>(typeArguments[i]);

		return key;
	}
}

SymbolTable::SymbolTable()
	: m_rootNamespace(m_arena.emplace<NamespaceSymbol>(L"", nullptr))
{
	// component order matters:
	// interfaces reference primitives,
	// printables and enumerables implement interfaces,
	// exceptions build on IThrowable
	// (async components stay stubbed until the async TODO lands)
	resolve_primitives();
	resolve_interfaces();
	resolve_primitive_printables();
	resolve_enumerables();
	resolve_exceptions();
}

gmt::Arena& SymbolTable::get_arena()
{
	return m_arena;
}

NamespaceSymbol* SymbolTable::get_root_namespace() const
{
	return m_rootNamespace;
}

SyntaxSymbol* SymbolTable::lookup_symbol(const SyntaxNode* node)
{
	auto choice = m_nodeToSymbolMap.find(node);
	if (choice == m_nodeToSymbolMap.end())
		return nullptr; // no symbol bound to this node

	return choice->second;
}

const SyntaxNode* SymbolTable::lookup_node(SyntaxSymbol* symbol)
{
	auto choice = m_symbolToNodeMap.find(symbol);
	if (choice == m_symbolToNodeMap.end())
		return nullptr; // no symbol bound to this node

	return choice->second;
}

void SymbolTable::bind_type_arguments(const SyntaxNode* node, std::span<StructSymbol* const> typeArguments)
{
	m_nodeTypeArguments[node].assign(typeArguments.begin(), typeArguments.end());
}

std::span<StructSymbol* const> SymbolTable::lookup_type_arguments(const SyntaxNode* node) const
{
	auto choice = m_nodeTypeArguments.find(node);
	if (choice == m_nodeTypeArguments.end())
		return {};

	return choice->second;
}

ArrayTypeSymbol* SymbolTable::find_array_type(StructSymbol* underlaying) const
{
	return find_array_type(underlaying, 0);
}

ArrayTypeSymbol* SymbolTable::find_array_type(StructSymbol* underlaying, std::size_t size) const
{
	const std::uint64_t key[3] = { ArrayFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), size };
	SyntaxSymbol* symbol = m_interner.find(key, 3);

	if ((symbol == nullptr) || symbol->get_kind() != SyntaxKind::ArrayType)
		return nullptr;

	return static_cast<ArrayTypeSymbol*>(symbol);
}

GenericTypeSymbol* SymbolTable::find_generic_type(StructSymbol* underlaying, std::span<StructSymbol* const> typeArguments) const
{
	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), typeArguments);

	SyntaxSymbol* symbol = m_interner.find(key, static_cast<std::uint16_t>(2 + typeArguments.size()));
	if ((symbol == nullptr) || symbol->get_kind() != SyntaxKind::GenericType)
		return nullptr;

	return static_cast<GenericTypeSymbol*>(symbol);
}

GenericMethodSymbol* SymbolTable::find_generic_method(MethodSymbol* underlaying, std::span<StructSymbol* const> typeArguments) const
{
	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericMethodFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), typeArguments);

	SyntaxSymbol* symbol = m_interner.find(key, static_cast<std::uint16_t>(2 + typeArguments.size()));
	if (symbol == nullptr)
		return nullptr;

	return static_cast<GenericMethodSymbol*>(symbol);
}

ArrayTypeSymbol* SymbolTable::find_or_create_array_type(StructSymbol* underlaying)
{
	return find_or_create_array_type(underlaying, 0);
}

ArrayTypeSymbol* SymbolTable::find_or_create_array_type(StructSymbol* underlaying, std::size_t size)
{
	ArrayTypeSymbol* existing = find_array_type(underlaying, size);
	if (!(existing == nullptr))
		return existing;

	ArrayTypeSymbol* symbol = m_arena.emplace<ArrayTypeSymbol>(underlaying, m_rootNamespace);
	symbol->set_length(size);

	const std::uint64_t key[3] = { ArrayFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), size };
	m_interner.insert(key, 3, symbol);
	return symbol;
}

GenericTypeSymbol* SymbolTable::find_or_create_generic_type(StructSymbol* underlaying, std::span<StructSymbol* const> typeArguments)
{
	GenericTypeSymbol* existing = find_generic_type(underlaying, typeArguments);
	if (!(existing == nullptr))
		return existing;

	GenericTypeSymbol* symbol = m_arena.emplace<GenericTypeSymbol>(underlaying, m_rootNamespace);

	const gmt::vector<TypeParameterSymbol*>& typeParameters = underlaying->type_parameters;
	for (std::size_t i = 0; i < typeParameters.size() && i < typeArguments.size(); ++i)
		symbol->add_type_parameter(typeParameters[i], typeArguments[i]);

	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), typeArguments);

	m_interner.insert(key, static_cast<std::uint16_t>(2 + typeArguments.size()), symbol);
	return symbol;
}

GenericMethodSymbol* SymbolTable::find_or_create_generic_method(MethodSymbol* underlaying, std::span<StructSymbol* const> typeArguments, StructSymbol* returnType)
{
	GenericMethodSymbol* existing = find_generic_method(underlaying, typeArguments);
	if (!(existing == nullptr))
		return existing;

	const SyntaxSymbol* parent = underlaying->get_parent();
	GenericMethodSymbol* symbol = m_arena.emplace<GenericMethodSymbol>(underlaying, const_cast<SyntaxSymbol*>(parent));
	symbol->set_type_arguments(typeArguments);
	symbol->set_return_type(returnType);
	symbol->set_is_virtual(underlaying->get_is_virtual());
	symbol->set_is_async(underlaying->get_is_async());
	symbol->set_linking(underlaying->get_linking());
	symbol->set_accesibility(underlaying->get_accesibility());

	// parameter symbols stay shared with the open method: the call protocol
	// only needs their count and order, types substitute per instantiation
	for (ParameterSymbol* parameter : underlaying->get_parameters())
		symbol->on_symbol_declared(parameter);

	std::uint64_t inlineKey[InlineKeyWords];
	gmt::inline_vector<std::uint64_t, 8> spilledKey;
	const std::uint64_t* key = make_generic_key(inlineKey, spilledKey.get_vector(), GenericMethodFamilyTag, reinterpret_cast<std::uint64_t>(underlaying), typeArguments);

	m_interner.insert(key, static_cast<std::uint16_t>(2 + typeArguments.size()), symbol);
	return symbol;
}

NullableTypeSymbol* SymbolTable::find_nullable_type(StructSymbol* underlaying) const
{
	const std::uint64_t key[2] = { NullableFamilyTag, reinterpret_cast<std::uint64_t>(underlaying) };
	SyntaxSymbol* symbol = m_interner.find(key, 2);

	if ((symbol == nullptr) || symbol->get_kind() != SyntaxKind::NullableType)
		return nullptr;

	return static_cast<NullableTypeSymbol*>(symbol);
}

NullableTypeSymbol* SymbolTable::find_or_create_nullable_type(StructSymbol* underlaying)
{
	NullableTypeSymbol* existing = find_nullable_type(underlaying);
	if (!(existing == nullptr))
		return existing;

	NullableTypeSymbol* symbol = m_arena.emplace<NullableTypeSymbol>(underlaying, m_rootNamespace);

	const std::uint64_t key[2] = { NullableFamilyTag, reinterpret_cast<std::uint64_t>(underlaying) };
	m_interner.insert(key, 2, symbol);
	return symbol;
}

StructSymbol* SymbolTable::get_void_primitive() const
{
	return m_void;
}

StructSymbol* SymbolTable::get_boolean_primitive() const
{
	return m_boolean;
}

StructSymbol* SymbolTable::get_integer_primitive() const
{
	return m_integer;
}

StructSymbol* SymbolTable::get_double_primitive() const
{
	return m_double;
}

StructSymbol* SymbolTable::get_char_primitive() const
{
	return m_char;
}

StructSymbol* SymbolTable::get_string_primitive() const
{
	return m_string;
}

StructSymbol* SymbolTable::get_array_primitive() const
{
	return m_array;
}

StructSymbol* SymbolTable::get_nint_primitive() const
{
	return m_nativeInteger;
}

StructSymbol* SymbolTable::get_byte_primitive() const
{
	return m_byte;
}

SyntaxSymbol* SymbolTable::get_standard_type(StandardTypes type)
{
	SyntaxSymbol*& slot = m_standardTypes[static_cast<std::size_t>(type)];

	/*
	if (slot == nullptr)
		slot = m_arena.emplace<StructSymbol>(L"", SyntaxKind::StructDeclaration, m_rootNamespace);
	*/

	return slot;
}

void SymbolTable::set_standard_type(StandardTypes type, SyntaxSymbol* symbol)
{
	m_standardTypes[static_cast<std::size_t>(type)] = symbol;
}
