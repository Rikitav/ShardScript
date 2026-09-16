#include <shard/parsing/nodes/statements/ContinueStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ContinueStatementSyntax::ContinueStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::ContinueStatement, parent) { }

SyntaxToken ContinueStatementSyntax::get_continue_keyword() const
{
	return m_continueKeywordToken;
}

SyntaxToken ContinueStatementSyntax::get_tag() const
{
	return m_tagToken;
}

SyntaxToken ContinueStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ContinueStatementSyntax::set_continue_keyword(const SyntaxToken& token)
{
	m_continueKeywordToken = token;
}

void ContinueStatementSyntax::set_tag(const SyntaxToken& token)
{
	m_tagToken = token;
}

void ContinueStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ContinueStatementSyntax::get_location() const
{
	return TextLocation(m_continueKeywordToken, m_semicolonToken);
}

void ContinueStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_continue_statement(this);
}
