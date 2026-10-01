#include <shard/parsing/nodes/loops/ForInStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ForInStatementSyntax::ForInStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::ForInStatement, parent) { }

SyntaxToken ForInStatementSyntax::get_for_keyword() const
{
	return m_forKeywordToken;
}

SyntaxToken ForInStatementSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken ForInStatementSyntax::get_in_keyword() const
{
	return m_inKeywordToken;
}

gmt::Ref<const ExpressionSyntax> ForInStatementSyntax::get_range_expression() const
{
	return m_rangeExpression;
}

gmt::Ref<const StatementsBlockSyntax> ForInStatementSyntax::get_block() const
{
	return m_block;
}

void ForInStatementSyntax::set_for_keyword(const SyntaxToken& token)
{
	m_forKeywordToken = token;
}

void ForInStatementSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void ForInStatementSyntax::set_in_keyword(const SyntaxToken& token)
{
	m_inKeywordToken = token;
}

void ForInStatementSyntax::set_range_expression(gmt::Ref<ExpressionSyntax> rangeExpression)
{
	m_rangeExpression = rangeExpression;
}

void ForInStatementSyntax::set_block(gmt::Ref<StatementsBlockSyntax> block)
{
	m_block = block;
}

TextLocation ForInStatementSyntax::get_location() const
{
	if (!m_block.is_null())
		return TextLocation(m_forKeywordToken, m_block.as_ptr()->get_location());

	return m_forKeywordToken.get_location();
}

void ForInStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_for_in_statement(this);
}
