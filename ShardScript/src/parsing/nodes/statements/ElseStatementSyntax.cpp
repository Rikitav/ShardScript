#include <shard/parsing/nodes/statements/ElseStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ElseStatementSyntax::ElseStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: ConditionalClauseBaseSyntax(SyntaxKind::ElseStatement, parent) { }

SyntaxToken ElseStatementSyntax::get_else_keyword() const
{
	return m_elseKeywordToken;
}

void ElseStatementSyntax::set_else_keyword(const SyntaxToken& token)
{
	m_elseKeywordToken = token;
}

TextLocation ElseStatementSyntax::get_location() const
{
	if (!get_next().is_null())
		return TextLocation(m_elseKeywordToken, get_next().as_ptr()->get_location());

	if (!get_block().is_null())
		return TextLocation(m_elseKeywordToken, get_block().as_ptr()->get_location());

	return m_elseKeywordToken.get_location();
}

void ElseStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_else_statement(this);
}
