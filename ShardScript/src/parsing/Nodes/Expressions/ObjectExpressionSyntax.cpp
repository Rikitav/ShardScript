#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ObjectExpressionSyntax::ObjectExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

SyntaxToken ObjectExpressionSyntax::get_new_token() const
{
	return m_newToken;
}

const TypeSyntax* ObjectExpressionSyntax::get_type() const
{
	return m_type;
}

const ArgumentsListSyntax* ObjectExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void ObjectExpressionSyntax::set_new_token(const SyntaxToken& token)
{
	m_newToken = token;
}

void ObjectExpressionSyntax::set_type(TypeSyntax* type)
{
	m_type = type;
}

void ObjectExpressionSyntax::set_arguments(ArgumentsListSyntax* arguments)
{
	m_arguments = arguments;
}

const ExpressionSyntax* ObjectExpressionSyntax::get_array_size() const
{
	return m_arraySize;
}

bool ObjectExpressionSyntax::is_array_creation() const
{
	return !(m_arraySize == nullptr);
}

void ObjectExpressionSyntax::set_array_size(ExpressionSyntax* arraySize)
{
	m_arraySize = arraySize;
}

TextLocation ObjectExpressionSyntax::get_location() const
{
	if (!(m_arguments == nullptr))
		return TextLocation(m_newToken, m_arguments->get_close_token());

	if (!(m_arraySize == nullptr))
		return TextLocation(m_newToken, m_arraySize->get_location());

	if (!(m_type == nullptr))
		return TextLocation(m_newToken, m_type->get_location());

	return m_newToken.get_location();
}

void ObjectExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_object_creation_expression(this);
}