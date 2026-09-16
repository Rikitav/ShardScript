#include <shard/parsing/nodes/types/GenericTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

#include <sstream>

using namespace shard;

GenericTypeSyntax::GenericTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::GenericType, parent) { }

gmt::Ref<const TypeSyntax> GenericTypeSyntax::get_underlaying_type() const
{
	return m_underlayingType;
}

gmt::Ref<const TypeArgumentsListSyntax> GenericTypeSyntax::get_type_arguments() const
{
	return m_typeArguments;
}

void GenericTypeSyntax::set_underlaying_type(gmt::Ref<TypeSyntax> underlayingType)
{
	m_underlayingType = underlayingType;
}

void GenericTypeSyntax::set_type_arguments(gmt::Ref<TypeArgumentsListSyntax> typeArguments)
{
	m_typeArguments = typeArguments;
}

std::wstring GenericTypeSyntax::get_qualifier() const
{
	if (m_underlayingType.is_null())
		return L"";

	std::wstringstream qualifier;
	qualifier << m_underlayingType.as_ptr()->get_qualifier() << L"<";

	std::span<const gmt::Ref<TypeSyntax>> arguments = m_typeArguments.as_ptr()->get_types().as_span();
	if (!arguments.empty())
	{
		for (std::size_t i = 0; i < arguments.size() - 1; ++i)
			qualifier << arguments[i].as_ptr()->get_qualifier() << L", ";

		qualifier << arguments.back().as_ptr()->get_qualifier();
	}

	qualifier << L">";
	return qualifier.str();
}

TextLocation GenericTypeSyntax::get_location() const
{
	if (!m_underlayingType.is_null())
	{
		if (!m_typeArguments.is_null())
			return TextLocation(m_underlayingType.as_ptr()->get_location(), m_typeArguments.as_ptr()->get_close_token());

		return m_underlayingType.as_ptr()->get_location();
	}

	return TextLocation();
}

void GenericTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_generic_type(this);
}
