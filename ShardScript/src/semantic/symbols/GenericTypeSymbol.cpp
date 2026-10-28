#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

using namespace shard;

GenericTypeSymbol::GenericTypeSymbol(StructSymbol* underlayingType, SyntaxSymbol* parent)
	: StructSymbol(gmt::intern(underlayingType->get_name()), SyntaxKind::GenericType, parent), m_underlayingType(underlayingType) { }

StructSymbol* GenericTypeSymbol::get_underlaying_type() const
{
	return m_underlayingType;
}

bool GenericTypeSymbol::add_type_parameter(TypeParameterSymbol* typeParameter, StructSymbol* argumentType)
{
	return m_typeArguments.try_emplace(gmt::intern(typeParameter->get_name()), argumentType);
}

StructSymbol* GenericTypeSymbol::substitute_type_parameter(TypeParameterSymbol* typeParameter) const
{
	// type parameters substitute by name, so a foreign parameter that happens
	// to share a name with one of this type's own (a method's T against the
	// receiver's T) must not resolve through this construction
	bool owned = false;
	for (TypeParameterSymbol* parameter : m_underlayingType->type_parameters)
	{
		if (parameter == typeParameter)
		{
			owned = true;
			break;
		}
	}

	if (!owned)
		return nullptr;

	StructSymbol* const* argument = m_typeArguments.find(gmt::intern(typeParameter->get_name()));
	if (argument == nullptr)
		return nullptr;

	return *argument;
}