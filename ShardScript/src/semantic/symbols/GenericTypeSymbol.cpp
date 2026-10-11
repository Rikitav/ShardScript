#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

GenericTypeSymbol::GenericTypeSymbol(gmt::Ref<StructSymbol> underlaying_type, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(gmt::intern(underlaying_type->get_name()), SyntaxKind::GenericType, parent), m_underlayingType(underlaying_type) { }

gmt::Ref<StructSymbol> GenericTypeSymbol::get_underlaying_type() const
{
	return m_underlayingType;
}

bool GenericTypeSymbol::add_type_parameter(gmt::Ref<TypeParameterSymbol> type_parameter, gmt::Ref<StructSymbol> argument_type)
{
	return m_typeArguments.try_emplace(gmt::intern(type_parameter->get_name()), argument_type);
}

gmt::Ref<StructSymbol> GenericTypeSymbol::substitute_type_parameter(gmt::Ref<TypeParameterSymbol> type_parameter) const
{
	const gmt::Ref<StructSymbol>* argument = m_typeArguments.find(gmt::intern(type_parameter->get_name()));
	if (argument == nullptr)
		return gmt::nullref;

	return *argument;
}
