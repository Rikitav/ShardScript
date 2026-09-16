#include <shard/parsing/nodes/statements/DeferVariableSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DeferVariableSyntax::DeferVariableSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::DeferVariable, parent) { }

SyntaxToken DeferVariableSyntax::get_defer_keyword() const
{
	return m_deferKeywordToken;
}

gmt::Ref<const VariableStatementSyntax> DeferVariableSyntax::get_variable() const
{
	return m_variable;
}

void DeferVariableSyntax::set_defer_keyword(const SyntaxToken& token)
{
	m_deferKeywordToken = token;
}

void DeferVariableSyntax::set_variable(gmt::Ref<VariableStatementSyntax> variable)
{
	m_variable = variable;
}

TextLocation DeferVariableSyntax::get_location() const
{
	if (!m_variable.is_null())
		return TextLocation(m_deferKeywordToken, m_variable.as_ptr()->get_location());

	return m_deferKeywordToken.get_location();
}

void DeferVariableSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_defer_variable(this);
}
