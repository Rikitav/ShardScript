#include <shard/parsing/nodes/statements/CatchClauseSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

CatchClauseSyntax::CatchClauseSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::CatchClause, parent) { }

SyntaxToken CatchClauseSyntax::get_catch_keyword() const
{
	return m_catchKeywordToken;
}

SyntaxToken CatchClauseSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken CatchClauseSyntax::get_colon() const
{
	return m_colonToken;
}

gmt::Ref<const TypeSyntax> CatchClauseSyntax::get_exception_type() const
{
	return m_exceptionType;
}

gmt::Ref<const StatementsBlockSyntax> CatchClauseSyntax::get_body() const
{
	return m_body;
}

void CatchClauseSyntax::set_catch_keyword(const SyntaxToken& token)
{
	m_catchKeywordToken = token;
}

void CatchClauseSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void CatchClauseSyntax::set_colon(const SyntaxToken& token)
{
	m_colonToken = token;
}

void CatchClauseSyntax::set_exception_type(gmt::Ref<TypeSyntax> exceptionType)
{
	m_exceptionType = exceptionType;
}

void CatchClauseSyntax::set_body(gmt::Ref<StatementsBlockSyntax> body)
{
	m_body = body;
}

TextLocation CatchClauseSyntax::get_location() const
{
	if (!m_body.is_null())
		return TextLocation(m_catchKeywordToken, m_body.as_ptr()->get_location());

	if (!m_exceptionType.is_null())
		return TextLocation(m_catchKeywordToken, m_exceptionType.as_ptr()->get_location());

	return m_catchKeywordToken.get_location();
}

void CatchClauseSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_catch_clause(this);
}
