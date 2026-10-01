#include <shard/parsing/nodes/statements/ConditionalClauseSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ConditionalClauseSyntax::ConditionalClauseSyntax(SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: ConditionalClauseBaseSyntax(kind, parent) { }

gmt::Ref<const ExpressionSyntax> ConditionalClauseSyntax::get_condition() const
{
	return m_condition;
}

void ConditionalClauseSyntax::set_condition(gmt::Ref<ExpressionSyntax> condition)
{
	m_condition = condition;
}
