#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>
#include <gmt/Containers.hpp>

#include <sstream>

using namespace shard;

GenericTypeSyntax::GenericTypeSyntax(SyntaxNode* parent)
	: TypeSyntax(SyntaxKind::GenericType, parent) { }

const TypeSyntax* GenericTypeSyntax::get_underlaying_type() const
{
	return m_underlayingType;
}

const TypeArgumentsListSyntax* GenericTypeSyntax::get_type_arguments() const
{
	return m_typeArguments;
}

void GenericTypeSyntax::set_underlaying_type(TypeSyntax* underlayingType)
{
	m_underlayingType = underlayingType;
}

void GenericTypeSyntax::set_type_arguments(TypeArgumentsListSyntax* typeArguments)
{
	m_typeArguments = typeArguments;
}

gmt::wstring GenericTypeSyntax::get_qualifier() const
{
	if (m_underlayingType == nullptr)
		return L"";

	std::wstringstream qualifier;
	qualifier << m_underlayingType->get_qualifier() << L"<";

	std::span<TypeSyntax* const> arguments = m_typeArguments->get_types();
	if (!arguments.empty())
	{
		for (std::size_t i = 0; i < arguments.size() - 1; ++i)
			qualifier << arguments[i]->get_qualifier() << L", ";

		qualifier << arguments.back()->get_qualifier();
	}

	qualifier << L">";
	return gmt::wstring(qualifier.str());
}

TextLocation GenericTypeSyntax::get_location() const
{
	if (!(m_underlayingType == nullptr))
	{
		if (!(m_typeArguments == nullptr))
			return TextLocation(m_underlayingType->get_location(), m_typeArguments->get_close_token());

		return m_underlayingType->get_location();
	}

	return TextLocation();
}

void GenericTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_generic_type(this);
}
