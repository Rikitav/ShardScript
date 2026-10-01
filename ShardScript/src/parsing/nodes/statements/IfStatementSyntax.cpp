#include <shard/parsing/nodes/statements/IfStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IfStatementSyntax::IfStatementSyntax(gmt::Ref<SyntaxNode> parent)
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
	if (!get_next().is_null())
		return TextLocation(m_ifKeywordToken, get_next().as_ptr()->get_location());

	if (!get_block().is_null())
		return TextLocation(m_ifKeywordToken, get_block().as_ptr()->get_location());

	return m_ifKeywordToken.get_location();
}

void IfStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_if_statement(this);
}
