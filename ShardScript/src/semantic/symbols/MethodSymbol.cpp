#include <shard/semantic/symbols/MethodSymbol.hpp>

#include <gmt/Containers.hpp>
#include <gmt/Interner.hpp>

#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>

using namespace shard;

MethodSymbol::MethodSymbol(std::wstring_view name, SyntaxSymbol* parent)
	: MethodSymbol(gmt::intern(name), parent) { }

MethodSymbol::MethodSymbol(gmt::StringReference name, SyntaxSymbol* parent)
	: MemberSymbol(name, SyntaxKind::FunctionDeclaration, parent) { }

MethodSymbol::MethodSymbol(std::wstring_view name, const SyntaxKind kind, SyntaxSymbol* parent)
	: MethodSymbol(gmt::intern(name), kind, parent) { }

MethodSymbol::MethodSymbol(gmt::StringReference name, const SyntaxKind kind, SyntaxSymbol* parent)
	: MemberSymbol(name, kind, parent) { }

CallingConvention MethodSymbol::get_calling_convention() const
{
	return m_callingConvention;
}

void MethodSymbol::set_calling_convention(CallingConvention callingConvention)
{
	m_callingConvention = callingConvention;
}

void* MethodSymbol::get_function_pointer() const
{
	return m_functionPointer;
}

void MethodSymbol::set_function_pointer(void* functionPointer)
{
	m_functionPointer = functionPointer;
}

StructSymbol* MethodSymbol::get_return_type() const
{
	return m_returnType;
}

void MethodSymbol::set_return_type(StructSymbol* returnType)
{
	m_returnType = returnType;
}

const gmt::vector<TypeParameterSymbol*>& MethodSymbol::get_type_parameters() const
{
	return m_typeParameters;
}

const gmt::vector<ParameterSymbol*>& MethodSymbol::get_parameters() const
{
	return m_parameters;
}

bool MethodSymbol::get_is_virtual() const
{
	return m_isVirtual;
}

void MethodSymbol::set_is_virtual(bool isVirtual)
{
	m_isVirtual = isVirtual;
}

bool MethodSymbol::get_is_async() const
{
	return m_isAsync;
}

void MethodSymbol::set_is_async(bool isAsync)
{
	m_isAsync = isAsync;
}

MethodEffectSummary& MethodSymbol::get_effect_summary()
{
	return m_effectSummary;
}

const MethodEffectSummary& MethodSymbol::get_effect_summary() const
{
	return m_effectSummary;
}

bool MethodSymbol::get_effects_computed() const
{
	return m_effectsComputed;
}

void MethodSymbol::set_effects_computed(bool effectsComputed)
{
	m_effectsComputed = effectsComputed;
}

bool MethodSymbol::is_method() const
{
	return true;
}

bool MethodSymbol::get_is_generic_instantiation() const
{
	return false;
}

void MethodSymbol::on_symbol_declared(SyntaxSymbol* symbol)
{
	switch (symbol->get_kind())
	{
		case SyntaxKind::TypeParameter:
		{
			m_typeParameters.push_back(static_cast<TypeParameterSymbol*>(symbol));
			break;
		}

		case SyntaxKind::Parameter:
		{
			m_parameters.push_back(static_cast<ParameterSymbol*>(symbol));
			break;
		}

		default:
			break;
	}
}
