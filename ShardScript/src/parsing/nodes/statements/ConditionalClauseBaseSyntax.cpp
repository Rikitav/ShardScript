#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ConditionalClauseBaseSyntax::ConditionalClauseBaseSyntax(SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(kind, parent) { }

gmt::Ref<const StatementsBlockSyntax> ConditionalClauseBaseSyntax::get_block() const
{
	return m_block;
}

gmt::Ref<const ConditionalClauseBaseSyntax> ConditionalClauseBaseSyntax::get_next() const
{
	return m_next;
}

void ConditionalClauseBaseSyntax::set_block(gmt::Ref<StatementsBlockSyntax> block)
{
	m_block = block;
}

void ConditionalClauseBaseSyntax::set_next(gmt::Ref<ConditionalClauseBaseSyntax> next)
{
	m_next = next;
}
