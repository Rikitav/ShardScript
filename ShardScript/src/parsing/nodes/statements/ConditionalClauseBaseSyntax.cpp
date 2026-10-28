#include <shard/parsing/nodes/statements/ConditionalClauseBaseSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ConditionalClauseBaseSyntax::ConditionalClauseBaseSyntax(SyntaxKind kind, SyntaxNode* parent)
	: StatementSyntax(kind, parent) { }

const StatementsBlockSyntax* ConditionalClauseBaseSyntax::get_block() const
{
	return m_block;
}

const ConditionalClauseBaseSyntax* ConditionalClauseBaseSyntax::get_next() const
{
	return m_next;
}

void ConditionalClauseBaseSyntax::set_block(StatementsBlockSyntax* block)
{
	m_block = block;
}

void ConditionalClauseBaseSyntax::set_next(ConditionalClauseBaseSyntax* next)
{
	m_next = next;
}