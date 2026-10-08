#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Interner.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

using namespace shard;

MethodSymbol::MethodSymbol(std::wstring_view name, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(gmt::intern(name), parent) { }

MethodSymbol::MethodSymbol(gmt::StringReference name, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, SyntaxKind::FunctionDeclaration, parent) { }

MethodSymbol::MethodSymbol(std::wstring_view name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(gmt::intern(name), kind, parent) { }

MethodSymbol::MethodSymbol(gmt::StringReference name, const SyntaxKind kind, gmt::Ref<SyntaxSymbol> parent)
	: MemberSymbol(name, kind, parent) { }

gmt::Ref<StructSymbol> MethodSymbol::get_return_type() const
{
	return m_returnType;
}

void MethodSymbol::set_return_type(gmt::Ref<StructSymbol> return_type)
{
	m_returnType = return_type;
}

const std::vector<gmt::Ref<TypeParameterSymbol>>& MethodSymbol::get_type_parameters() const
{
	return m_typeParameters;
}

const std::vector<gmt::Ref<ParameterSymbol>>& MethodSymbol::get_parameters() const
{
	return m_parameters;
}

bool MethodSymbol::get_is_abstract() const
{
	return m_isAbstract;
}

void MethodSymbol::set_is_abstract(bool is_abstract)
{
	m_isAbstract = is_abstract;
}

bool MethodSymbol::get_is_async() const
{
	return m_isAsync;
}

void MethodSymbol::set_is_async(bool is_async)
{
	m_isAsync = is_async;
}

bool MethodSymbol::is_method() const
{
	return true;
}

void MethodSymbol::on_symbol_declared(gmt::Ref<SyntaxSymbol> symbol)
{
	switch (symbol->get_kind())
	{
		case SyntaxKind::TypeParameter:
		{
			m_typeParameters.push_back(symbol.as<TypeParameterSymbol>());
			break;
		}

		case SyntaxKind::Parameter:
		{
			m_parameters.push_back(symbol.as<ParameterSymbol>());
			break;
		}
	}
}
