#include <shard/parsing/nodes/members/FunctionDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

FunctionDeclarationSyntax::FunctionDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeDeclarationSyntax(SyntaxKind::FunctionDeclaration, parent) { }

gmt::Ref<const BodySyntax> FunctionDeclarationSyntax::get_body() const
{
	return m_body;
}

gmt::Ref<const TypeSyntax> FunctionDeclarationSyntax::get_return_type() const
{
	return m_returnType;
}

gmt::Ref<const ParametersListSyntax> FunctionDeclarationSyntax::get_parameters_list() const
{
	return m_parametersList;
}

void FunctionDeclarationSyntax::set_body(gmt::Ref<BodySyntax> body)
{
	m_body = body;
}

void FunctionDeclarationSyntax::set_return_type(gmt::Ref<TypeSyntax> returnType)
{
	m_returnType = returnType;
}

void FunctionDeclarationSyntax::set_parameters_list(gmt::Ref<ParametersListSyntax> parametersList)
{
	m_parametersList = parametersList;
}

TextLocation FunctionDeclarationSyntax::get_location() const
{
	TextLocation location = get_declare_token().get_location();

	if (!get_body().is_null())
		return TextLocation(location, get_body().as_ptr()->get_location());

	if (!get_semicolon().is_missing())
		return TextLocation(location, get_semicolon().get_location());

	if (!get_identifier().is_missing())
		return TextLocation(location, get_identifier().get_location());

	return location;
}

void FunctionDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_function_declaration(this);
}
