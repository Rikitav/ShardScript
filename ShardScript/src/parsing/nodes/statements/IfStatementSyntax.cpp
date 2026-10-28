#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IfStatementSyntax::IfStatementSyntax(SyntaxNode* parent)
	: ConditionalClauseSyntax(SyntaxKind::IfStatement, parent) { }

SyntaxToken IfStatementSyntax::get_if_keyword() const
{
	return m_ifKeywordToken;
}

void IfStatementSyntax::set_if_keyword(const SyntaxToken& token)
{
	m_ifKeywordToken = token;
}

TextLocation IfStatementSyntax::get_location() const
{
	if (!(get_next() == nullptr))
		return TextLocation(m_ifKeywordToken, get_next()->get_location());

	if (!(get_block() == nullptr))
		return TextLocation(m_ifKeywordToken, get_block()->get_location());

	return m_ifKeywordToken.get_location();
}

void IfStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_if_statement(this);
}