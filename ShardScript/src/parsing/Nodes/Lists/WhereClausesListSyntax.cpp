#include <shard/parsing/nodes/lists/WhereClausesListSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

WhereClauseSyntax::WhereClauseSyntax(SyntaxNode* parent)
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

std::span<TypeSyntax* const> WhereClauseSyntax::get_constraint_types() const
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

void WhereClauseSyntax::set_constraint_types(std::span<TypeSyntax*> constraintTypes)
{
	m_constraintTypes = constraintTypes;
}

TextLocation WhereClauseSyntax::get_location() const
{
	if (!m_constraintTypes.empty())
	{
		std::span<TypeSyntax* const> constraints = m_constraintTypes;
		return TextLocation(m_whereKeywordToken, constraints.back()->get_location());
	}

	return TextLocation(m_whereKeywordToken, m_identifierToken);
}

void WhereClauseSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_where_clause(this);
}

WhereClausesListSyntax::WhereClausesListSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::WhereClausesList, parent) { }

std::span<WhereClauseSyntax* const> WhereClausesListSyntax::get_clauses() const
{
	return m_clauses;
}

void WhereClausesListSyntax::set_clauses(std::span<WhereClauseSyntax*> clauses)
{
	m_clauses = clauses;
}

TextLocation WhereClausesListSyntax::get_location() const
{
	if (!m_clauses.empty())
	{
		std::span<WhereClauseSyntax* const> clauses = m_clauses;
		return TextLocation(clauses.front()->get_location(), clauses.back()->get_location());
	}

	return TextLocation();
}

void WhereClausesListSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_where_clauses_list(this);
}