#include <shard/parsing/nodes/loops/WhileStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

WhileStatementSyntax::WhileStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::WhileStatement, parent) { }

SyntaxToken WhileStatementSyntax::get_while_keyword() const
{
	return m_whileKeywordToken;
}

SyntaxToken WhileStatementSyntax::get_open_curl() const
{
	return m_openCurlToken;
}

SyntaxToken WhileStatementSyntax::get_close_curl() const
{
	return m_closeCurlToken;
}

gmt::Ref<const ExpressionSyntax> WhileStatementSyntax::get_condition() const
{
	return m_condition;
}

gmt::Ref<const StatementsBlockSyntax> WhileStatementSyntax::get_block() const
{
	return m_block;
}

void WhileStatementSyntax::set_while_keyword(const SyntaxToken& token)
{
	m_whileKeywordToken = token;
}

void WhileStatementSyntax::set_open_curl(const SyntaxToken& token)
{
	m_openCurlToken = token;
}

void WhileStatementSyntax::set_close_curl(const SyntaxToken& token)
{
	m_closeCurlToken = token;
}

void WhileStatementSyntax::set_condition(gmt::Ref<ExpressionSyntax> condition)
{
	m_condition = condition;
}

void WhileStatementSyntax::set_block(gmt::Ref<StatementsBlockSyntax> block)
{
	m_block = block;
}

TextLocation WhileStatementSyntax::get_location() const
{
	if (!m_block.is_null())
		return TextLocation(m_whileKeywordToken, m_block.as_ptr()->get_location());

	return m_whileKeywordToken.get_location();
}

void WhileStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_while_statement(this);
}
