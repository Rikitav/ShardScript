#include <shard/semantic/SymbolTable.hpp>

using namespace shard;

SymbolTable::SymbolTable()
	: m_rootNamespace(m_arena.emplace<NamespaceSymbol>(L"", gmt::nullref))
{ }

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

	std::size_t hash_generic_key(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments)
	{
		std::size_t hash = hash_combine(0, underlaying.get_offset());
		for (const gmt::Ref<StructSymbol>& argument : type_arguments)
			hash = hash_combine(hash, argument.get_offset());

		return hash;
	}

	bool same_array_shape(gmt::Ref<ArrayTypeSymbol> symbol, gmt::Ref<StructSymbol> underlaying, std::size_t size)
	{
		return symbol->get_underlaying_type() == underlaying && symbol->get_length() == size;
	}

	bool same_generic_shape(gmt::Ref<GenericTypeSymbol> symbol, gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments)
	{
		if (symbol->get_underlaying_type() != underlaying)
			return false;

		const std::vector<gmt::Ref<TypeParameterSymbol>>& type_parameters = underlaying->type_parameters;
		if (type_parameters.size() != type_arguments.size())
			return false;

		for (std::size_t i = 0; i < type_parameters.size(); ++i)
		{
			if (symbol->substitute_type_parameter(type_parameters[i]) != type_arguments[i])
				return false;
		}

		return true;
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

gmt::Ref<GenericTypeSymbol> SymbolTable::find_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments) const
{
	const auto bucket = m_genericTypes.find(hash_generic_key(underlaying, type_arguments));
	if (bucket == m_genericTypes.end())
		return gmt::nullref;

	for (const gmt::Ref<GenericTypeSymbol>& candidate : bucket->second)
	{
		if (same_generic_shape(candidate, underlaying, type_arguments))
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

gmt::Ref<GenericTypeSymbol> SymbolTable::find_or_create_generic_type(gmt::Ref<StructSymbol> underlaying, const std::vector<gmt::Ref<StructSymbol>>& type_arguments)
{
	gmt::Ref<GenericTypeSymbol> existing = find_generic_type(underlaying, type_arguments);
	if (!existing.is_null())
		return existing;

	gmt::Ref<GenericTypeSymbol> symbol = m_arena.emplace<GenericTypeSymbol>(underlaying, m_rootNamespace);

	const std::vector<gmt::Ref<TypeParameterSymbol>>& type_parameters = underlaying->type_parameters;
	for (std::size_t i = 0; i < type_parameters.size() && i < type_arguments.size(); ++i)
		symbol->add_type_parameter(type_parameters[i], type_arguments[i]);

	m_genericTypes[hash_generic_key(underlaying, type_arguments)].push_back(symbol);
	return symbol;
}

gmt::Ref<StructSymbol> SymbolTable::get_void_primitive()
{
	return m_void;
}

gmt::Ref<StructSymbol> SymbolTable::get_boolean_primitive()
{
	return m_boolean;
}

gmt::Ref<StructSymbol> SymbolTable::get_integer_primitive()
{
	return m_integer;
}

gmt::Ref<StructSymbol> SymbolTable::get_double_primitive()
{
	return m_double;
}

gmt::Ref<StructSymbol> SymbolTable::get_char_primitive()
{
	return m_char;
}

gmt::Ref<StructSymbol> SymbolTable::get_string_primitive()
{
	return m_string;
}

gmt::Ref<StructSymbol> SymbolTable::get_nint_primitive()
{
	return m_nativeInteger;
}

gmt::Ref<StructSymbol> SymbolTable::get_byte_primitive()
{
	return m_byte;
}
