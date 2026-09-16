#include <shard/parsing/nodes/expressions/ObjectExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ObjectExpressionSyntax::ObjectExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::ObjectExpression, parent) { }

SyntaxToken ObjectExpressionSyntax::get_new_token() const
{
	return m_newToken;
}

gmt::Ref<const TypeSyntax> ObjectExpressionSyntax::get_type() const
{
	return m_type;
}

gmt::Ref<const ArgumentsListSyntax> ObjectExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void ObjectExpressionSyntax::set_new_token(const SyntaxToken& token)
{
	m_newToken = token;
}

void ObjectExpressionSyntax::set_type(gmt::Ref<TypeSyntax> type)
{
	m_type = type;
}

void ObjectExpressionSyntax::set_arguments(gmt::Ref<ArgumentsListSyntax> arguments)
{
	m_arguments = arguments;
}

gmt::Ref<const ExpressionSyntax> ObjectExpressionSyntax::get_array_size() const
{
	return m_arraySize;
}

bool ObjectExpressionSyntax::is_array_creation() const
{
	return !m_arraySize.is_null();
}

void ObjectExpressionSyntax::set_array_size(gmt::Ref<ExpressionSyntax> arraySize)
{
	m_arraySize = arraySize;
}

TextLocation ObjectExpressionSyntax::get_location() const
{
	if (!m_arguments.is_null())
		return TextLocation(m_newToken, m_arguments.as_ptr()->get_close_token());

	if (!m_arraySize.is_null())
		return TextLocation(m_newToken, m_arraySize.as_ptr()->get_location());

	if (!m_type.is_null())
		return TextLocation(m_newToken, m_type.as_ptr()->get_location());

	return m_newToken.get_location();
}

void ObjectExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_object_creation_expression(this);
}
