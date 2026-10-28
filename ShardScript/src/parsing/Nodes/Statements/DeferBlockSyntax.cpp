#include <shard/parsing/nodes/statements/DeferBlockSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DeferBlockSyntax::DeferBlockSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::DeferBlock, parent) { }

SyntaxToken DeferBlockSyntax::get_defer_keyword() const
{
	return m_deferKeywordToken;
}

const StatementsBlockSyntax* DeferBlockSyntax::get_block() const
{
	return m_block;
}

void DeferBlockSyntax::set_defer_keyword(const SyntaxToken& token)
{
	m_deferKeywordToken = token;
}

void DeferBlockSyntax::set_block(StatementsBlockSyntax* block)
{
	m_block = block;
}

TextLocation DeferBlockSyntax::get_location() const
{
	if (!(m_block == nullptr))
		return TextLocation(m_deferKeywordToken, m_block->get_location());

	return m_deferKeywordToken.get_location();
}

void DeferBlockSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_defer_block(this);
}