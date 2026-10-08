#include <shard/semantic/symbols/OperatorSymbol.hpp>

#include <gmt/Interner.hpp>

using namespace shard;

OperatorSymbol::OperatorSymbol(std::wstring_view name, const TokenType operator_token, gmt::Ref<SyntaxSymbol> parent)
	: OperatorSymbol(gmt::intern(name), operator_token, parent) { }

OperatorSymbol::OperatorSymbol(gmt::StringReference name, const TokenType operator_token, gmt::Ref<SyntaxSymbol> parent)
	: MethodSymbol(name, SyntaxKind::OperatorDeclaration, parent), m_operatorToken(operator_token) { }

TokenType OperatorSymbol::get_operator_token() const
{
	return m_operatorToken;
}

void OperatorSymbol::set_operator_token(const TokenType operator_token)
{
	m_operatorToken = operator_token;
}
