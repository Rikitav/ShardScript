#include <shard/parsing/nodes/statements/UnlessStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

UnlessStatementSyntax::UnlessStatementSyntax(SyntaxNode* parent)
	: ConditionalClauseSyntax(SyntaxKind::UnlessStatement, parent) { }

SyntaxToken UnlessStatementSyntax::get_unless_keyword() const
{
	return m_unlessKeywordToken;
}

void UnlessStatementSyntax::set_unless_keyword(const SyntaxToken& token)
{
	m_unlessKeywordToken = token;
}

TextLocation UnlessStatementSyntax::get_location() const
{
	if (!(get_next() == nullptr))
		return TextLocation(m_unlessKeywordToken, get_next()->get_location());

	if (!(get_block() == nullptr))
		return TextLocation(m_unlessKeywordToken, get_block()->get_location());

	return m_unlessKeywordToken.get_location();
}

void UnlessStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_unless_statement(this);
}