#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ConditionalClauseSyntax::ConditionalClauseSyntax(SyntaxKind kind, SyntaxNode* parent)
	: ConditionalClauseBaseSyntax(kind, parent) { }

const ExpressionSyntax* ConditionalClauseSyntax::get_condition() const
{
	return m_condition;
}

void ConditionalClauseSyntax::set_condition(ExpressionSyntax* condition)
{
	m_condition = condition;
}