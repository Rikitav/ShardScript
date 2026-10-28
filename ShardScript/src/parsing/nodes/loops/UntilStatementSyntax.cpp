#include <shard/parsing/nodes/loops/UntilStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

UntilStatementSyntax::UntilStatementSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::UntilStatement, parent) { }

SyntaxToken UntilStatementSyntax::get_until_keyword() const
{
	return m_untilKeywordToken;
}

const ExpressionSyntax* UntilStatementSyntax::get_condition() const
{
	return m_condition;
}

const StatementsBlockSyntax* UntilStatementSyntax::get_block() const
{
	return m_block;
}

void UntilStatementSyntax::set_until_keyword(const SyntaxToken& token)
{
	m_untilKeywordToken = token;
}

void UntilStatementSyntax::set_condition(ExpressionSyntax* condition)
{
	m_condition = condition;
}

void UntilStatementSyntax::set_block(StatementsBlockSyntax* block)
{
	m_block = block;
}

TextLocation UntilStatementSyntax::get_location() const
{
	if (!(m_block == nullptr))
		return TextLocation(m_untilKeywordToken, m_block->get_location());

	return m_untilKeywordToken.get_location();
}

void UntilStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_until_statement(this);
}