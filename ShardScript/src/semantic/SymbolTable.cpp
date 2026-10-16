#include <shard/semantic/SymbolTable.hpp>

#include <shard/parsing/SyntaxKind.hpp>

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
	inline std::size_t hash_combine(std::size_t seed, std::size_t value)
	{
		return seed ^ (value + 0x9e3779b9u + (seed << 6) + (seed >> 2));
	}

	std::size_t hash_array_key(gmt::Ref<StructSymbol> underlaying, std::size_t size)
	{
		std::size_t hash = hash_combine(0, underlaying.get_offset());
		return hash_combine(hash, size);
	}

	std::size_t hash_generic_key(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& typeArguments)
	{
		std::size_t hash = hash_combine(0, underlaying.get_offset());
		for (const gmt::Ref<StructSymbol>& argument : typeArguments)
			hash = hash_combine(hash, argument.get_offset());

		return hash;
	}

	bool same_array_shape(gmt::Ref<ArrayTypeSymbol> symbol, gmt::Ref<StructSymbol> underlaying, std::size_t size)
	{
		return symbol->get_underlaying_type() == underlaying && symbol->get_length() == size;
	}

	bool same_generic_shape(gmt::Ref<GenericTypeSymbol> symbol, gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& typeArguments)
	{
		if (symbol->get_underlaying_type() != underlaying)
			return false;

		const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = underlaying->type_parameters;
		if (typeParameters.size() != typeArguments.size())
			return false;

		for (std::size_t i = 0; i < typeParameters.size(); ++i)
		{
			if (symbol->substitute_type_parameter(typeParameters[i]) != typeArguments[i])
				return false;
		}

		return true;
	}

	std::size_t hash_nullable_key(gmt::Ref<StructSymbol> underlaying)
	{
		return hash_combine(0, underlaying.get_offset());
	}

	bool same_nullable_shape(gmt::Ref<NullableTypeSymbol> symbol, gmt::Ref<StructSymbol> underlaying)
	{
		return symbol->get_underlaying_type() == underlaying;
	}
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_array_type(gmt::Ref<StructSymbol> underlaying) const
{
	return find_array_type(underlaying, 0);
}

gmt::Ref<ArrayTypeSymbol> SymbolTable::find_array_type(gmt::Ref<StructSymbol> underlaying, std::size_t size) const
{
	const auto bucket = m_arrayTypes.find(hash_array_key(underlaying, size));
	if (bucket == m_arrayTypes.end())
		return gmt::nullref;

	for (const gmt::Ref<ArrayTypeSymbol>& candidate : bucket->second)
	{
		if (same_array_shape(candidate, underlaying, size))
			return candidate;
	}

	return gmt::nullref;
}

gmt::Ref<GenericTypeSymbol> SymbolTable::find_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& typeArguments) const
{
	const auto bucket = m_genericTypes.find(hash_generic_key(underlaying, typeArguments));
	if (bucket == m_genericTypes.end())
		return gmt::nullref;

	for (const gmt::Ref<GenericTypeSymbol>& candidate : bucket->second)
	{
		if (same_generic_shape(candidate, underlaying, typeArguments))
			return candidate;
	}

	return gmt::nullref;
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

	m_arrayTypes[hash_array_key(underlaying, size)].push_back(symbol);
	return symbol;
}

gmt::Ref<GenericTypeSymbol> SymbolTable::find_or_create_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& typeArguments)
{
	gmt::Ref<GenericTypeSymbol> existing = find_generic_type(underlaying, typeArguments);
	if (!existing.is_null())
		return existing;

	gmt::Ref<GenericTypeSymbol> symbol = m_arena.emplace<GenericTypeSymbol>(underlaying, m_rootNamespace);

	const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = underlaying->type_parameters;
	for (std::size_t i = 0; i < typeParameters.size() && i < typeArguments.size(); ++i)
		symbol->add_type_parameter(typeParameters[i], typeArguments[i]);
	m_genericTypes[hash_generic_key(underlaying, typeArguments)].push_back(symbol);
	return symbol;
}

gmt::Ref<NullableTypeSymbol> SymbolTable::find_nullable_type(gmt::Ref<StructSymbol> underlaying) const
{
	const auto bucket = m_nullableTypes.find(hash_nullable_key(underlaying));
	if (bucket == m_nullableTypes.end())
		return gmt::nullref;

	for (const gmt::Ref<NullableTypeSymbol>& candidate : bucket->second)
	{
		if (same_nullable_shape(candidate, underlaying))
			return candidate;
	}

	return gmt::nullref;
}

gmt::Ref<NullableTypeSymbol> SymbolTable::find_or_create_nullable_type(gmt::Ref<StructSymbol> underlaying)
{
	gmt::Ref<NullableTypeSymbol> existing = find_nullable_type(underlaying);
	if (!existing.is_null())
		return existing;

	gmt::Ref<NullableTypeSymbol> symbol = m_arena.emplace<NullableTypeSymbol>(underlaying, m_rootNamespace);
	m_nullableTypes[hash_nullable_key(underlaying)].push_back(symbol);
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
