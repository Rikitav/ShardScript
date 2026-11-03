#include <shard/parsing/nodes/expressions/DefaultExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DefaultExpressionSyntax::DefaultExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::DefaultExpression, parent) { }

SyntaxToken DefaultExpressionSyntax::get_default_keyword() const
{
	return m_defaultKeyword;
}

const TypeSyntax* DefaultExpressionSyntax::get_type() const
{
	return m_type;
}

void DefaultExpressionSyntax::set_default_keyword(const SyntaxToken& token)
{
	m_defaultKeyword = token;
}

void DefaultExpressionSyntax::set_type(TypeSyntax* type)
{
	m_type = type;
}

TextLocation DefaultExpressionSyntax::get_location() const
{
	if (m_type != nullptr)
		return TextLocation(m_defaultKeyword.get_location(), m_type->get_location());

	return m_defaultKeyword.get_location();
}

void DefaultExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_default_expression(this);
}
