#include <shard/parsing/nodes/statements/BreakStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BreakStatementSyntax::BreakStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::BreakStatement, parent) { }

SyntaxToken BreakStatementSyntax::get_break_keyword() const
{
	return m_breakKeywordToken;
}

SyntaxToken BreakStatementSyntax::get_tag() const
{
	return m_tagToken;
}

SyntaxToken BreakStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void BreakStatementSyntax::set_break_keyword(const SyntaxToken& token)
{
	m_breakKeywordToken = token;
}

void BreakStatementSyntax::set_tag(const SyntaxToken& token)
{
	m_tagToken = token;
}

void BreakStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation BreakStatementSyntax::get_location() const
{
	return TextLocation(m_breakKeywordToken, m_semicolonToken);
}

void BreakStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_break_statement(this);
}
