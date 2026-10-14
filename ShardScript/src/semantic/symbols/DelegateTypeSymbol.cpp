#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

DelegateTypeSymbol::DelegateTypeSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: DelegateTypeSymbol(gmt::intern(name), parent) { }

DelegateTypeSymbol::DelegateTypeSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: StructSymbol(name, SyntaxKind::DelegateType, parent) { }

gmt::Ref<StructSymbol> DelegateTypeSymbol::get_return_type() const
{
	return m_returnType;
}

void DelegateTypeSymbol::set_return_type(gmt::Ref<StructSymbol> returnType)
{
	m_returnType = returnType;
}

const std::vector<gmt::Ref<ParameterSymbol>>& DelegateTypeSymbol::get_parameters() const
{
	return m_parameters;
}

void DelegateTypeSymbol::set_parameters(const std::vector<gmt::Ref<ParameterSymbol>>& parameters)
{
	m_parameters = parameters;
}

const std::vector<gmt::Ref<TypeParameterSymbol>>& DelegateTypeSymbol::get_type_parameters() const
{
	return m_typeParameters;
}

void DelegateTypeSymbol::set_type_parameters(const std::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters)
{
	m_typeParameters = typeParameters;
}

bool DelegateTypeSymbol::get_is_async() const
{
	return m_isAsync;
}

void DelegateTypeSymbol::set_is_async(bool isAsync)
{
	m_isAsync = isAsync;
}
