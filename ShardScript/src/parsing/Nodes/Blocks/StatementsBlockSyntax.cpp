#include <shard/parsing/nodes/blocks/StatementsBlockSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

StatementsBlockSyntax::StatementsBlockSyntax(gmt::Ref<SyntaxNode> parent)
	: BodySyntax(SyntaxKind::StatementsBlock, parent) { }

gmt::Span<const gmt::Ref<StatementSyntax>> StatementsBlockSyntax::get_statements() const
{
	return m_statements;
}

SyntaxToken StatementsBlockSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken StatementsBlockSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

void StatementsBlockSyntax::set_statements(gmt::Span<gmt::Ref<StatementSyntax>> statements)
{
	m_statements = statements;
}

void StatementsBlockSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void StatementsBlockSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

TextLocation StatementsBlockSyntax::get_location() const
{
	return m_openBracketToken.get_location();
}

void StatementsBlockSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_statements_block(this);
}
