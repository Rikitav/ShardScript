#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DeferVariableSyntax::DeferVariableSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::DeferVariable, parent) { }

SyntaxToken DeferVariableSyntax::get_defer_keyword() const
{
	return m_deferKeywordToken;
}

const VariableStatementSyntax* DeferVariableSyntax::get_variable() const
{
	return m_variable;
}

void DeferVariableSyntax::set_defer_keyword(const SyntaxToken& token)
{
	m_deferKeywordToken = token;
}

void DeferVariableSyntax::set_variable(VariableStatementSyntax* variable)
{
	m_variable = variable;
}

TextLocation DeferVariableSyntax::get_location() const
{
	if (!(m_variable == nullptr))
		return TextLocation(m_deferKeywordToken, m_variable->get_location());

	return m_deferKeywordToken.get_location();
}

void DeferVariableSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_defer_variable(this);
}