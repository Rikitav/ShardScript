#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

GenericTypeSymbol::GenericTypeSymbol(gmt::Ref<StructSymbol> underlayingType, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(gmt::intern(underlayingType->get_name()), SyntaxKind::GenericType, parent), m_underlayingType(underlayingType) { }

gmt::Ref<StructSymbol> GenericTypeSymbol::get_underlaying_type() const
{
	return m_underlayingType;
}

bool GenericTypeSymbol::add_type_parameter(gmt::Ref<TypeParameterSymbol> typeParameter, gmt::Ref<StructSymbol> argumentType)
{
	return m_typeArguments.try_emplace(gmt::intern(typeParameter->get_name()), argumentType);
}

gmt::Ref<StructSymbol> GenericTypeSymbol::substitute_type_parameter(gmt::Ref<TypeParameterSymbol> typeParameter) const
{
	const gmt::Ref<StructSymbol>* argument = m_typeArguments.find(gmt::intern(typeParameter->get_name()));
	if (argument == nullptr)
		return gmt::nullref;

	return *argument;
}
