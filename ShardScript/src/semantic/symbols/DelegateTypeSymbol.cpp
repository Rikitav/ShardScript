#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Interner.hpp>

using namespace shard;

DelegateTypeSymbol::DelegateTypeSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: DelegateTypeSymbol(gmt::intern(name), parent) { }

DelegateTypeSymbol::DelegateTypeSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: StructSymbol(name, SyntaxKind::DelegateType, parent) { }

StructSymbol* DelegateTypeSymbol::get_return_type() const
{
	return m_returnType;
}

void DelegateTypeSymbol::set_return_type(StructSymbol* returnType)
{
	m_returnType = returnType;
}

const gmt::vector<ParameterSymbol*>& DelegateTypeSymbol::get_parameters() const
{
	return m_parameters;
}

void DelegateTypeSymbol::set_parameters(const gmt::vector<ParameterSymbol*>& parameters)
{
	m_parameters = parameters;
}

const gmt::vector<TypeParameterSymbol*>& DelegateTypeSymbol::get_type_parameters() const
{
	return type_parameters;
}

void DelegateTypeSymbol::set_type_parameters(const gmt::vector<TypeParameterSymbol*>& typeParameters)
{
	type_parameters = typeParameters;
}

bool DelegateTypeSymbol::get_is_async() const
{
	return m_isAsync;
}

void DelegateTypeSymbol::set_is_async(bool isAsync)
{
	m_isAsync = isAsync;
}
