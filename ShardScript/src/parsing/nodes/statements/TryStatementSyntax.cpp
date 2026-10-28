#include <shard/parsing/nodes/statements/TryStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

TryStatementSyntax::TryStatementSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::TryStatement, parent) { }

SyntaxToken TryStatementSyntax::get_try_keyword() const
{
	return m_tryKeywordToken;
}

const StatementsBlockSyntax* TryStatementSyntax::get_try_block() const
{
	return m_tryBlock;
}

std::span<CatchClauseSyntax* const> TryStatementSyntax::get_catch_clauses() const
{
	return m_catchClauses;
}

void TryStatementSyntax::set_try_keyword(const SyntaxToken& token)
{
	m_tryKeywordToken = token;
}

void TryStatementSyntax::set_try_block(StatementsBlockSyntax* tryBlock)
{
	m_tryBlock = tryBlock;
}

void TryStatementSyntax::set_catch_clauses(std::span<CatchClauseSyntax*> catchClauses)
{
	m_catchClauses = catchClauses;
}

TextLocation TryStatementSyntax::get_location() const
{
	if (m_catchClauses.size() > 0 && !(m_catchClauses.back() == nullptr))
		return TextLocation(m_tryKeywordToken, m_catchClauses.back()->get_location());

	if (!(m_tryBlock == nullptr))
		return TextLocation(m_tryKeywordToken, m_tryBlock->get_location());

	return m_tryKeywordToken.get_location();
}

void TryStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_try_statement(this);
}