#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

WhereClauseSyntax::WhereClauseSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::WhereClause, parent) { }

SyntaxToken WhereClauseSyntax::get_where_keyword() const
{
	return m_whereKeywordToken;
}

SyntaxToken WhereClauseSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken WhereClauseSyntax::get_colon() const
{
	return m_colonToken;
}

gmt::Span<const gmt::Ref<TypeSyntax>> WhereClauseSyntax::get_constraint_types() const
{
	return m_constraintTypes;
}

void WhereClauseSyntax::set_where_keyword(const SyntaxToken& token)
{
	m_whereKeywordToken = token;
}

void WhereClauseSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void WhereClauseSyntax::set_colon(const SyntaxToken& token)
{
	m_colonToken = token;
}

void WhereClauseSyntax::set_constraint_types(gmt::Span<gmt::Ref<TypeSyntax>> constraintTypes)
{
	m_constraintTypes = constraintTypes;
}

TextLocation WhereClauseSyntax::get_location() const
{
	if (!m_constraintTypes.empty())
	{
		std::span<const gmt::Ref<TypeSyntax>> constraints = m_constraintTypes.get();
		return TextLocation(m_whereKeywordToken, constraints.back().get()->get_location());
	}

	return TextLocation(m_whereKeywordToken, m_identifierToken);
}

void WhereClauseSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_where_clause(this);
}

WhereClausesListSyntax::WhereClausesListSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::WhereClausesList, parent) { }

gmt::Span<const gmt::Ref<WhereClauseSyntax>> WhereClausesListSyntax::get_clauses() const
{
	return m_clauses;
}

void WhereClausesListSyntax::set_clauses(gmt::Span<gmt::Ref<WhereClauseSyntax>> clauses)
{
	m_clauses = clauses;
}

TextLocation WhereClausesListSyntax::get_location() const
{
	if (!m_clauses.empty())
	{
		std::span<const gmt::Ref<WhereClauseSyntax>> clauses = m_clauses.get();
		return TextLocation(clauses.front().get()->get_location(), clauses.back().get()->get_location());
	}

	return TextLocation();
}

void WhereClausesListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_where_clauses_list(this);
}
