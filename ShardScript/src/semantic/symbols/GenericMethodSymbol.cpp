#include <shard/semantic/symbols/GenericMethodSymbol.hpp>

#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

GenericMethodSymbol::GenericMethodSymbol(gmt::Ref<MethodSymbol> underlayingMethod, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(gmt::intern(underlayingMethod->get_name()), SyntaxKind::FunctionDeclaration, parent), m_underlayingMethod(underlayingMethod) { }

gmt::Ref<MethodSymbol> GenericMethodSymbol::get_underlaying_method() const
{
	return m_underlayingMethod;
}

bool GenericMethodSymbol::get_is_generic_instantiation() const
{
	return true;
}

const std::vector<gmt::Ref<StructSymbol>>& GenericMethodSymbol::get_type_arguments() const
{
	return m_typeArguments;
}

void GenericMethodSymbol::set_type_arguments(std::span<const gmt::Ref<StructSymbol>> typeArguments)
{
	m_typeArguments.assign(typeArguments.begin(), typeArguments.end());
}
