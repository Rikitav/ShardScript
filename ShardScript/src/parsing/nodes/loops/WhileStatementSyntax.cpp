#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

WhileStatementSyntax::WhileStatementSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::WhileStatement, parent) { }

SyntaxToken WhileStatementSyntax::get_while_keyword() const
{
	return m_whileKeywordToken;
}

const ExpressionSyntax* WhileStatementSyntax::get_condition() const
{
	return m_condition;
}

const StatementsBlockSyntax* WhileStatementSyntax::get_block() const
{
	return m_block;
}

void WhileStatementSyntax::set_while_keyword(const SyntaxToken& token)
{
	m_whileKeywordToken = token;
}

void WhileStatementSyntax::set_condition(ExpressionSyntax* condition)
{
	m_condition = condition;
}

void WhileStatementSyntax::set_block(StatementsBlockSyntax* block)
{
	m_block = block;
}

TextLocation WhileStatementSyntax::get_location() const
{
	if (!(m_block == nullptr))
		return TextLocation(m_whileKeywordToken, m_block->get_location());

	return m_whileKeywordToken.get_location();
}

void WhileStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_while_statement(this);
}